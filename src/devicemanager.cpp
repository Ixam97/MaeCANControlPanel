/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <ixam97@ixam97> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 *
 * ----------------------------------------------------------------------------
 * https://github.com/Ixam97
 * ----------------------------------------------------------------------------
 * MäCAN Control Panel
 * devicemanager.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-18.1]
 */

#define T_SLIDER 1
#define T_DROPDOWN 2

#include "devicemanager.h"
#include "configworker.h"
#include "updater.h"
#include "imgui.h"
#include "guihelpers.h"

#include <sstream>
#include <queue>
#include <chrono>

uint32_t m_update_uid;
uint16_t m_update_type;
std::string m_update_name;
bool m_draw_updater = false;

std::queue<Globals::CanFrame> m_frame_out_queue;

namespace DeviceManager 
{
    void drawUpdateInfo(ImFont* _bold_font);

    void loop()
    {
        std::chrono::steady_clock::time_point now_time = std::chrono::high_resolution_clock::now();
        static std::chrono::steady_clock::time_point last_request_time = now_time;
        std::chrono::duration<double> request_duration = now_time - last_request_time;

        // Request readings channel info in regular intervals or if new entry was added to the request list.
        if (Globals::ProgramStates::new_request_list_entry || (request_duration.count() >= Globals::ProgramSettings::request_interval))
        {
            Globals::ProgramStates::new_request_list_entry = false;
            last_request_time = now_time;
            for (ConfigWorker::readingsRequestInfo n : ConfigWorker::readings_request_list)
            {
                uint8_t tmp_data[8] = { (uint8_t)(n.uid >> 24), (uint8_t)(n.uid >> 16),(uint8_t)(n.uid >> 8), (uint8_t)n.uid, SYS_STAT, n.channel,0,0 };
                m_frame_out_queue.push(Globals::CanFrame(CMD_SYS, 0, 6, tmp_data));
            }
        }

        // When update started, repeat update offer until aborted or client is found
        if (MCANUpdater::UpdateInterface::status == MCAN_UPDATE_INIT)
        {
            static auto _last_time_sent = std::chrono::high_resolution_clock::now();
            auto _current_time = std::chrono::high_resolution_clock::now();

            if (_current_time - _last_time_sent > std::chrono::milliseconds(750))
            {
                MCANUpdater::repeatUpdateOffer();
                _last_time_sent = _current_time;
            }
        }
        // ConfigWorker constructs config channels for gui
        if (!ConfigWorker::busy() && !MCANUpdater::busy())
        {
            // Get first UID from device list without aditional info and pass to configWorker
            for (int i = 0; i < ConfigWorker::device_list.size(); i++)
            {
                if (ConfigWorker::device_list[i].num_config_channels == -1)
                {
                    ConfigWorker::workOn(ConfigWorker::device_list[i].uid);
                    break;
                }
            }
        }

        // Reset congfigWorker when device list is cleared
        if (Globals::ProgramCmds::config_worker_reset)
        {
            Globals::ProgramCmds::config_worker_reset = false;
            ConfigWorker::device_list.resize(0);
            ConfigWorker::readings_request_list.resize(0);
            ConfigWorker::reset();
        }

        // Updater Interface
        if (MCANUpdater::UpdateInterface::get_file_names)
        {
            MCANUpdater::UpdateInterface::get_file_names = false;
            MCANUpdater::getFileNames(MCANUpdater::UpdateInterface::file_names);
        }
        if (MCANUpdater::UpdateInterface::do_update)
        {
            MCANUpdater::UpdateInterface::do_update = false;
            if (!MCANUpdater::busy() && !ConfigWorker::busy())
                MCANUpdater::UpdateInterface::status = MCANUpdater::startUpdate(MCANUpdater::UpdateInterface::uid, MCANUpdater::UpdateInterface::type, MCANUpdater::UpdateInterface::file_name);
        }
        if (MCANUpdater::UpdateInterface::do_abort)
        {
            MCANUpdater::UpdateInterface::do_abort = false;
            if (MCANUpdater::busy())
                MCANUpdater::UpdateInterface::status = MCANUpdater::abortUpdate();
        }

        if (MCANUpdater::busy() != MCANUpdater::UpdateInterface::in_progress)
            MCANUpdater::UpdateInterface::in_progress = MCANUpdater::busy();

        if (MCANUpdater::getProgress() != MCANUpdater::UpdateInterface::progress)
            MCANUpdater::UpdateInterface::progress = MCANUpdater::getProgress();

        // Send config value requests if changed by gui
        for (ConfigWorker::canDevice& device : ConfigWorker::device_list)
        {
            for (ConfigWorker::configChannel& channel : device.vec_config_channels)
            {
                if (channel.request_sent == false)
                {
                    uint8_t tmp_data[8] = { (uint8_t)(device.uid >> 24), (uint8_t)(device.uid >> 16), (uint8_t)(device.uid >> 8), (uint8_t)(device.uid), SYS_STAT, channel.channel_index, (uint8_t)(channel.wanted_value >> 8), (uint8_t)(channel.wanted_value) };
                    m_frame_out_queue.push(Globals::CanFrame(CMD_SYS, 0, 8, tmp_data));
                    channel.request_sent = true;
                }
            }
        }
    }

    void draw(void* _bold_font, float _scaling)
    {

        static size_t current_index = 0;
        if (b_draw)
        {
            if (current_index >= ConfigWorker::device_list.size())
                current_index = ConfigWorker::device_list.size() - 1;
            if (!ImGui::Begin(u8"Gerätemanager", &b_draw, ImGuiWindowFlags_NoCollapse))
            {
                ImGui::End();
                return;
            }

            uint8_t tmp_data[8] = { 0,0,0,0,0,0,0,0 };
            if (ImGui::Button("Ping")) m_frame_out_queue.push(Globals::CanFrame(CMD_PING, 0, 0, tmp_data));
            ImGui::SameLine();
            if (ImGui::Button(u8"Liste zurücksetzen"))
            {
                current_index = 0;
                Globals::ProgramCmds::config_worker_reset = true;
            }

            ImGui::Separator();

            if (ImGui::BeginListBox("##", ImVec2(floor(200 * _scaling), -1)))
            {
                int iterator = 0;
                for (ConfigWorker::canDevice& device : ConfigWorker::device_list)
                {
                    if (device.data_complete)
                    {
                        const bool is_selected = (current_index == iterator);
                        std::stringstream ss_list_label;
                        ss_list_label << device.name << " #" << std::dec << device.serialnbr;
                        if (ImGui::Selectable(ss_list_label.str().c_str(), is_selected))
                        {
                            current_index = iterator;
                        }
                    }
                    iterator++;
                }
                ImGui::EndListBox();
            }
            ImGui::SameLine();
            if (ImGui::BeginChild("DeviceInfoRegion"))
            {
                if (ConfigWorker::device_list.size() > 0 && ConfigWorker::device_list[current_index].data_complete)
                {
                    auto& device = ConfigWorker::device_list[current_index];
                    bool b_is_not_mcan = !(device.type == 0x51 || device.type == 0x52 || device.type == 0x53 || device.type == 0x54 || device.type == 0x70);
                    ImGui::PushFont((ImFont*)_bold_font);
                    ImGui::Text(device.name.c_str());
                    ImGui::PopFont();
                    ImGui::Separator();
                    ImGui::Text("Artikelnummer: %s ", device.item.c_str());
                    ImGui::Text("Seriennummer: %d", device.serialnbr);
                    ImGui::Text("Version: %d.%d", device.version_h, device.version_l);
                    ImGui::Text("UID: 0x%08X", device.uid);
                    ImGui::Text(u8"Konfigurationskanäle: %d, Messwertkanäle: %d", device.num_config_channels, device.num_readings_channels);

                    if (ButtonDisablable("Update", b_is_not_mcan)) {
                        m_update_uid = device.uid;
                        m_update_type = device.type;
                        m_update_name = device.name;
                        m_draw_updater = true;
                    }
                    ImGui::SameLine();
                    if (ButtonDisablable("Reset", b_is_not_mcan))
                    {
                        uint32_t& tmp_uid = device.uid;
                        uint8_t tmp_data[8] = {(uint8_t)(tmp_uid >> 24), (uint8_t)(tmp_uid >> 16), (uint8_t)(tmp_uid >> 8), (uint8_t)(tmp_uid), 0,0,0,0};
                        m_frame_out_queue.push(Globals::CanFrame(CMD_MCAN_BOOT, 0, 4, tmp_data));
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(u8"Aus Liste löschen"))
                    {
                        ConfigWorker::removeFromDeviceList(current_index);
                        if (current_index >= ConfigWorker::device_list.size())
                            current_index--;
                    }
                    ImGui::PushItemWidth(200 * Globals::ProgramStates::gui_scaling);
                    if (device.num_readings_channels > 0)
                    {
                        ImGui::Separator();
                        for (ConfigWorker::readingsChannel n : device.vec_readings_channels)
                        {
                            float true_value = (n.current_value * n.value_factor) + std::stof(n.s_min);
                            float relative_value = (true_value - std::stof(n.s_min)) / (std::stof(n.s_max) - std::stof(n.s_min));
                            ImGui::Text(u8"%s: %.2f %s", n.label.c_str(), true_value, n.unit.c_str());

                            if (n.current_value > n.points[3]) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, byteToColor(n.colors[3]));
                            else if (n.current_value > n.points[2]) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, byteToColor(n.colors[2]));
                            else if (n.current_value > n.points[1]) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, byteToColor(n.colors[1]));
                            else ImGui::PushStyleColor(ImGuiCol_PlotHistogram, byteToColor(n.colors[0]));
                            ImGui::ProgressBar(relative_value, ImVec2(-1, 0), "");
                            ImGui::PopStyleColor();
                        }
                    }
                    if (device.num_config_channels > 0)
                    {
                        ImGui::Separator();
                        for (int i = 0; i < device.vec_config_channels.size(); i++)
                        {
                            //if (n.unit == "") ImGui::Text("%s", n.label.c_str());
                            //else ImGui::Text("%s (%s)", n.label.c_str(), n.unit.c_str());

                            // Drop down
                            if (device.vec_config_channels[i].type == 1)
                            {
                                if (ImGui::Combo(device.vec_config_channels[i].label.c_str(), &(device.vec_config_channels[i].wanted_value), device.vec_config_channels[i].dropdown_options_separated_by_zero.c_str()))
                                    device.vec_config_channels[i].request_sent = false;
                            }
                            // Int slider
                            else if (device.vec_config_channels[i].type == 2)
                            {
                                if (ImGui::SliderInt(device.vec_config_channels[i].label.c_str(), &(device.vec_config_channels[i].wanted_value), device.vec_config_channels[i].min, device.vec_config_channels[i].max))
                                    device.vec_config_channels[i].request_sent = false;
                                ImGui::SameLine(); helpMarker("STRG + Klick, um Zahlenwerte einzugeben.");
                            }
                        }
                    }
                    ImGui::PopItemWidth();
                }
            }
            ImGui::EndChild();

            ImGui::End();

            if (m_draw_updater)
                drawUpdateInfo((ImFont*)_bold_font);
        }
    }

    void drawUpdateInfo(ImFont* _bold_font)
    {
        if (ImGui::Begin(u8"MäCAN Updater", NULL, DialogWindowFlag)) {

            static bool filter_file_names = true;
            static bool file_names_read = false;
            static std::string s_file_names;
            static int selected_file_index = 0;
            static size_t last_file_names_size = 0;

            if (!MCANUpdater::UpdateInterface::get_file_names) MCANUpdater::UpdateInterface::get_file_names = true;

            if (last_file_names_size != MCANUpdater::UpdateInterface::file_names.size())
            {
                last_file_names_size = MCANUpdater::UpdateInterface::file_names.size();
                std::stringstream ss_file_names;
                for (std::string& s : MCANUpdater::UpdateInterface::file_names)
                {
                    ss_file_names << s << '\0';
                }
                s_file_names = ss_file_names.str();
                file_names_read = false;
            }

            ImGui::PushFont(_bold_font);
            ImGui::Text(u8"Update für %s", m_update_name.c_str());
            ImGui::PopFont();
            ImGui::Text("UID: 0x%08X, Typ 0x%02X", m_update_uid, m_update_type);
            ImGui::Separator();
            ImGui::Combo("HEX-Datei", &selected_file_index, s_file_names.c_str());

            ImGui::ProgressBar(MCANUpdater::UpdateInterface::progress);

            switch (MCANUpdater::UpdateInterface::status)
            {
            case MCAN_UPDATE_IDLE:
                ImGui::Text("Update nicht gestartet");
                break;
            case MCAN_UPDATE_IN_PROGRESS:
                ImGui::Text(u8"Update läuft");
                break;
            case MCAN_UPDATE_FAILURE_ERROR:
                ImGui::Text("Update wegen eines Fehlers abgebrochen");
                break;
            case MCAN_UPDATE_FAILURE_INCOMPATIBLE:
                ImGui::Text("Updater inkompatibel zum Bootloader");
                break;
            case MCAN_UPDATE_SUCCESS:
                ImGui::Text("Update erfolgreich abgeschlossen");
                break;
            case MCAN_UPDATE_INIT:
                ImGui::Text("Update wird eingeleitet");
                break;
            default:
                ImGui::Text("Unbekannter Status");
                break;
            }
            ImGui::Separator();

            if (ButtonDisablable("Update starten", MCANUpdater::UpdateInterface::file_names.size() < 1 || MCANUpdater::UpdateInterface::in_progress) && !MCANUpdater::UpdateInterface::in_progress)
            {
                MCANUpdater::UpdateInterface::file_name = MCANUpdater::UpdateInterface::file_names[selected_file_index];
                MCANUpdater::UpdateInterface::type = m_update_type;
                MCANUpdater::UpdateInterface::uid = m_update_uid;
                MCANUpdater::UpdateInterface::do_update = true;
            }
            ImGui::SameLine();
            if (ButtonDisablable("Update abbrechen", MCANUpdater::UpdateInterface::status != MCAN_UPDATE_INIT && MCANUpdater::UpdateInterface::status != MCAN_UPDATE_IN_PROGRESS) && MCANUpdater::UpdateInterface::in_progress)
                MCANUpdater::UpdateInterface::do_abort = true;
            ImGui::SameLine();
            if (ButtonDisablable(u8"Schließen", MCANUpdater::UpdateInterface::in_progress))
            {
                m_draw_updater = false;
                MCANUpdater::UpdateInterface::status = 0;
            }
        }
        ImGui::End();
    }

    void addFrame(Globals::CanFrame& _frame)
    {
        if (_frame.cmd == CMD_SYS)
        {
            // Reading/Config value response
            if (_frame.resp == 1 && _frame.dlc == 8 && _frame.data[4] == SYS_STAT)
            {
                uint32_t i_uid = Globals::getUidFromData(_frame.data);

                for (ConfigWorker::canDevice& device : ConfigWorker::device_list)
                {
                    if (device.uid == i_uid)
                    {
                        for (ConfigWorker::readingsChannel& channel : device.vec_readings_channels)
                        {
                            if (_frame.data[5] == channel.channel_index)
                            {
                                channel.current_value = (_frame.data[6] << 8) | _frame.data[7];
                                break;
                            }
                        }
                        for (ConfigWorker::configChannel& channel : device.vec_config_channels)
                        {
                            if (_frame.data[5] == channel.channel_index)
                            {
                                channel.current_value = (_frame.data[6] << 8) | _frame.data[7];
                                break;
                            }
                        }
                    }
                    if ((device.type == 0x0011 || device.type == 0x0010) && device.vec_readings_channels.size() > 1)
                    {
                        voltage = (float)(device.vec_readings_channels[1].current_value * device.vec_readings_channels[1].value_factor) + std::stof(device.vec_readings_channels[1].s_min);
                        current = (float)(device.vec_readings_channels[0].current_value * device.vec_readings_channels[0].value_factor) + std::stof(device.vec_readings_channels[0].s_min);
                    }
                }
            }

            // Config value request
            if (_frame.resp == 0 && _frame.dlc == 8 && _frame.data[4] == SYS_STAT)
            {
                uint32_t i_uid = Globals::getUidFromData(_frame.data);

                for (int i = 0; i < ConfigWorker::device_list.size(); i++)
                {
                    if (ConfigWorker::device_list[i].uid == i_uid)
                    {
                        for (int j = 0; j < ConfigWorker::device_list[i].vec_config_channels.size(); j++)
                        {
                            if (_frame.data[5] == ConfigWorker::device_list[i].vec_config_channels[j].channel_index)
                            {
                                ConfigWorker::device_list[i].vec_config_channels[j].wanted_value = (_frame.data[6] << 8) | _frame.data[7];
                                break;
                            }
                        }
                        break;
                    }
                }
            }

            // Config value confirmation
            if (_frame.resp == 1 && _frame.dlc == 7 && _frame.data[4] == SYS_STAT)
            {
                uint32_t i_uid = Globals::getUidFromData(_frame.data);

                for (int i = 0; i < ConfigWorker::device_list.size(); i++)
                {
                    if (ConfigWorker::device_list[i].uid == i_uid)
                    {
                        for (int j = 0; j < ConfigWorker::device_list[i].vec_config_channels.size(); j++)
                        {
                            if (_frame.data[5] == ConfigWorker::device_list[i].vec_config_channels[j].channel_index)
                            {
                                if (_frame.data[6] == 1)
                                    ConfigWorker::device_list[i].vec_config_channels[j].current_value = ConfigWorker::device_list[i].vec_config_channels[j].wanted_value;
                                else
                                    ConfigWorker::device_list[i].vec_config_channels[j].wanted_value = ConfigWorker::device_list[i].vec_config_channels[j].current_value;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
        if (_frame.cmd == CMD_PING)
        {
            // Ping response from other devices (Ignore CS2)
            if (_frame.resp == 1 && _frame.data[7] != 0xff)
            {
                // Check if device already on device list
                bool i_known_device = false;
                for (ConfigWorker::canDevice n : ConfigWorker::device_list)
                {
                    if (n.uid == Globals::getUidFromData(_frame.data))
                    {
                        i_known_device = true;
                        break;
                    }
                }

                if (!i_known_device)
                {
                    // Add device to device list
                    ConfigWorker::canDevice i_new_device;
                    i_new_device.uid = Globals::getUidFromData(_frame.data);
                    i_new_device.version_h = _frame.data[4];
                    i_new_device.version_l = _frame.data[5];
                    i_new_device.type = (_frame.data[6] << 8) | _frame.data[7];

                    ConfigWorker::device_list.push_back(i_new_device);
                }
            }
        }

        if (_frame.cmd == CMD_CONFIG)
            ConfigWorker::addFrame(_frame);
        if (_frame.cmd == CMD_MCAN_BOOT)
            MCANUpdater::addFrame(_frame);
    }

    bool getFrame(Globals::CanFrame& _frame)
    {
        Globals::CanFrame tmp_frame;
        while (ConfigWorker::getFrame(tmp_frame))
            m_frame_out_queue.push(tmp_frame);
        while (MCANUpdater::getFrame(tmp_frame))
            m_frame_out_queue.push(tmp_frame);

        if (m_frame_out_queue.size())
        {
            _frame = m_frame_out_queue.front();
            m_frame_out_queue.pop();
            return true;
        }
        return false;
    }
}
