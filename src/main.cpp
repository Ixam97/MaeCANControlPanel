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
 * M‰CAN Control Panel
 * main.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-07.2]
 */

#include "can.h"
#include "gui.h"
#include "globals.h"
#include "ini.h"
#include "configworker.h"
#include "updater.h"

#include <thread>
#include <chrono>

#define GLOBAL_SETTING_LOAD ini.get("global-settings")
#define GLOBAL_SETTING_WRITE ini["global-settings"]

#define CAN_FRAME_TIMEOUT 10 // Timeout in ms

mINI::INIFile file("MaeCANControlPanelSettings.ini");
mINI::INIStructure ini;

HWND consoleWindow;

void loadIni();
void writeIni();

bool done = false;

void logicLoop()
{

    while (!done || (CAN::getQueueLength(OUTQUEUE) > 0))
    {
        std::chrono::steady_clock::time_point now_time = std::chrono::high_resolution_clock::now();
        static std::chrono::steady_clock::time_point last_request_time = now_time;
        static std::chrono::steady_clock::time_point last_tcp_check_time = now_time;
        std::chrono::duration<double> request_duration = now_time - last_request_time;
        std::chrono::duration<double> tcp_check_duration = now_time - last_tcp_check_time;

        // Request readings channel info in regular intervals or if new entry was added to the request list.
        if (global_states.new_request_list_entry || (request_duration.count() >= global_settings.request_interval))
        {
            global_states.new_request_list_entry = false;
            last_request_time = now_time;
            for (readingsRequestInfo n : readings_request_list)
            {
                uint8_t i_data[8] = { (uint8_t)(n.uid >> 24), (uint8_t)(n.uid >> 16),(uint8_t)(n.uid >> 8), (uint8_t)n.uid, SYS_STAT, n.channel,0,0 };
                CAN::addFrameToQueue(newCanFrame(SYS_CMD, 0, 6, i_data), OUTQUEUE);
            }
        }

        // When update started, repeat update offer until aborted or client is found
        if (update_interfrace.status == MCAN_UPDATE_INIT)
        {
            static auto _last_time_sent = std::chrono::high_resolution_clock::now();
            auto _current_time = std::chrono::high_resolution_clock::now();

            if (_current_time - _last_time_sent > std::chrono::milliseconds(750))
            {
                MCANUpdater::repeatUpdateOffer();
                _last_time_sent = _current_time;
            }
        }


        if (tcp_check_duration.count() >= 5 && global_states.tcp_success)
        {
            last_tcp_check_time = now_time;
            CAN::TCPCheckConnection();
        }
            

        // Check connection establishment
        if (global_states.tcp_started) CAN::TCPCheckConnection();

        // Start connection establishment
        if (global_cmds.tcp_connect) 
        {
            global_cmds.tcp_connect = false;
            global_states.tcp_started = CAN::TCPConnect();
        }

        // Disconnect
        if (global_cmds.tcp_disconnect) 
        {
            global_cmds.tcp_disconnect = false;
            CAN::TCPDisconnect();
        }

        // Reset congfigWorker when device list is cleared
        if (global_cmds.config_worker_reset)
        {
            global_cmds.config_worker_reset = false;
            device_list.resize(0);
            readings_request_list.resize(0);
            ConfigWorker::reset();
        }

        // Save settings after they changed
        if (global_settings.has_changed)
        {
            if (global_settings.trace) ShowWindow(consoleWindow, SW_SHOW);
#ifndef _DEBUG
            else ShowWindow(consoleWindow, SW_HIDE);
#endif
            writeIni();
        }

        // ConfigWorker constructs config channels for gui
        if (!ConfigWorker::busy() && !MCANUpdater::busy())
        {
            // Get first UID from device list without aditional info and pass to configWorker
            for (int i = 0; i < device_list.size(); i++)
            {
                if (device_list[i].num_config_channels == -1)
                {
                    ConfigWorker::workOn(device_list[i].uid);
                    break;
                }
            }
        }

        // Updater Interface
        if (update_interfrace.get_file_names)
        {
            update_interfrace.get_file_names = false;
            MCANUpdater::getFileNames(update_interfrace.file_names);
        }
        if (update_interfrace.do_update)
        {
            update_interfrace.do_update = false;
            if(!MCANUpdater::busy() && !ConfigWorker::busy())
                update_interfrace.status = MCANUpdater::startUpdate(update_interfrace.uid, update_interfrace.type, update_interfrace.file_name);
        }
        if (update_interfrace.do_abort)
        {
            update_interfrace.do_abort = false;
            if (MCANUpdater::busy())
                update_interfrace.status = MCANUpdater::abortUpdate();
        }

        if (MCANUpdater::busy() != update_interfrace.in_progress)
            update_interfrace.in_progress = MCANUpdater::busy();

        if (MCANUpdater::getProgress() != update_interfrace.progress)
            update_interfrace.progress = MCANUpdater::getProgress();

        // Put new frames into queue
        CAN::TCPReadFrame();

        // Get new incoming frame from queue
        if (CAN::getQueueLength(INQUEUE))
        {
            canFrame new_in_frame = CAN::processQueue(INQUEUE);
            GUI::addFrameToConsoleVector(new_in_frame);

            switch (new_in_frame.cmd)
            {
            case SYS_CMD :
                // Track power state
                if (new_in_frame.resp == 1 && new_in_frame.dlc == 5)
                {
                    
                    if (new_in_frame.data[4] == SYS_GO) global_states.track_power = true;
                    else if (new_in_frame.data[4] == SYS_STOP) global_states.track_power = false;

                }

                // Reading/Config value response
                if (new_in_frame.resp == 1 && new_in_frame.dlc == 8 && new_in_frame.data[4] == SYS_STAT)
                {
                    uint32_t i_uid = getUidFromData(new_in_frame.data);

                    for (int i = 0; i < device_list.size(); i++)
                    {
                        if (device_list[i].uid == i_uid)
                        {
                            for (int j = 0; j < device_list[i].vec_readings_channels.size(); j++)
                            {
                                if (new_in_frame.data[5] == device_list[i].vec_readings_channels[j].channel_index)
                                {
                                    device_list[i].vec_readings_channels[j].current_value = (new_in_frame.data[6] << 8) | new_in_frame.data[7];
                                    break;
                                }
                            }
                            for (int j = 0; j < device_list[i].vec_config_channels.size(); j++)
                            {
                                if (new_in_frame.data[5] == device_list[i].vec_config_channels[j].channel_index)
                                {
                                    device_list[i].vec_config_channels[j].current_value = (new_in_frame.data[6] << 8) | new_in_frame.data[7];
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }

                // Config value request
                if (new_in_frame.resp == 0 && new_in_frame.dlc == 8 && new_in_frame.data[4] == SYS_STAT)
                {
                    uint32_t i_uid = getUidFromData(new_in_frame.data);

                    for (int i = 0; i < device_list.size(); i++)
                    {
                        if (device_list[i].uid == i_uid)
                        {
                            for (int j = 0; j < device_list[i].vec_config_channels.size(); j++)
                            {
                                if (new_in_frame.data[5] == device_list[i].vec_config_channels[j].channel_index)
                                {
                                    device_list[i].vec_config_channels[j].wanted_value = (new_in_frame.data[6] << 8) | new_in_frame.data[7];
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }

                // Config value confirmation
                if (new_in_frame.resp == 1 && new_in_frame.dlc == 7 && new_in_frame.data[4] == SYS_STAT)
                {
                    uint32_t i_uid = getUidFromData(new_in_frame.data);

                    for (int i = 0; i < device_list.size(); i++)
                    {
                        if (device_list[i].uid == i_uid)
                        {
                            for (int j = 0; j < device_list[i].vec_config_channels.size(); j++)
                            {
                                if (new_in_frame.data[5] == device_list[i].vec_config_channels[j].channel_index)
                                {
                                    if (new_in_frame.data[6] == 1)
                                        device_list[i].vec_config_channels[j].current_value = device_list[i].vec_config_channels[j].wanted_value;
                                    else
                                        device_list[i].vec_config_channels[j].wanted_value = device_list[i].vec_config_channels[j].current_value;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            case CMD_PING:
                // Ping response from other devices (Ignore CS2)
                if (new_in_frame.resp == 1 && new_in_frame.data[7] != 0xff)
                {
                    // Check if device already on device list
                    bool i_known_device = false;
                    for (canDevice n : device_list)
                    {
                        if (n.uid == getUidFromData(new_in_frame.data))
                        {
                            i_known_device = true;
                            break;
                        }
                    }

                    if (!i_known_device)
                    {
                        // Add device to device list
                        canDevice i_new_device;
                        i_new_device.uid = getUidFromData(new_in_frame.data);
                        i_new_device.version_h = new_in_frame.data[4];
                        i_new_device.version_l = new_in_frame.data[5];
                        i_new_device.type = (new_in_frame.data[6] << 8) | new_in_frame.data[7];

                        device_list.push_back(i_new_device);
                    }

                }
                break;
            case CMD_CONFIG :
                // Config data frames
                if (new_in_frame.resp == 1 && ConfigWorker::busy())
                {
                    // Pass config data frame to configWorker to process
                    ConfigWorker::addFrame(new_in_frame);
                }
                break;
            case CMD_MCAN_BOOT :
                if (new_in_frame.resp == 1 && MCANUpdater::busy())
                {
                    update_interfrace.status = MCANUpdater::addFrame(new_in_frame);
                }
                break;
            default:
                break;
            }
        }

        // Send config value requests if changed by gui
        for (int i = 0; i < device_list.size(); i++)
        {
            for (int j = 0; j < device_list[i].vec_config_channels.size(); j++)
            {
                if (device_list[i].vec_config_channels[j].request_sent == false)
                {
                    uint8_t frame_data[] = { (uint8_t)(device_list[i].uid >> 24), (uint8_t)(device_list[i].uid >> 16), (uint8_t)(device_list[i].uid >> 8), (uint8_t)(device_list[i].uid), SYS_STAT, device_list[i].vec_config_channels[j].channel_index, (uint8_t)(device_list[i].vec_config_channels[j].wanted_value >> 8), (uint8_t)(device_list[i].vec_config_channels[j].wanted_value) };
                    CAN::addFrameToQueue(newCanFrame(SYS_CMD, 0, 8, frame_data), OUTQUEUE);
                    device_list[i].vec_config_channels[j].request_sent = true;
                }
            }
        }

        // Interfacing with other modules using CAN
        canFrame tmp_frame;
        while (GUI::getFrame(tmp_frame))
        {
            CAN::addFrameToQueue(tmp_frame, OUTQUEUE);
        }
        while (ConfigWorker::getFrame(tmp_frame))
        {
            CAN::addFrameToQueue(tmp_frame, OUTQUEUE);
        }
        while (MCANUpdater::getFrame(tmp_frame))
        {
            CAN::addFrameToQueue(tmp_frame, OUTQUEUE);
        }

        // Work on Queue to send
        if (CAN::getQueueLength(OUTQUEUE))
        {
            // Send queued frames after timeout
            static auto _last_time_sent = std::chrono::high_resolution_clock::now();
            auto _current_time = std::chrono::high_resolution_clock::now();

            if (_current_time - _last_time_sent > std::chrono::milliseconds(CAN_FRAME_TIMEOUT))
            {
                _last_time_sent = _current_time;
                canFrame newOutFrame = CAN::processQueue(OUTQUEUE);
                if (newOutFrame.can_hash > 0) GUI::addFrameToConsoleVector(newOutFrame);
            }
        }
        Sleep(1);
    }
}

// Main code
int main(int argc, char** argv)
{
    loadIni();

    consoleWindow = GetConsoleWindow();
#ifndef _DEBUG
    if (!global_settings.trace) ShowWindow(consoleWindow, SW_HIDE);
#endif
    GUI::GUISetup(1900, 900, u8"M‰CAN Control Panel");

    // Put logic in seperate thread to not block it when resizing or moving the GUI window
    std::thread logicThread(logicLoop);

    // GUI loop
    while (!done) {
        GUI::poll(&done);
        GUI::newFrame();
        GUI::drawMainWindow();
        GUI::render();
    }

    //ShowWindow(consoleWindow, SW_SHOW);

    logicThread.join();

    GUI::cleanup();
    CAN::TCPDisconnect();


    return 0;
}

void loadIni() 
{
    if (file.read(ini)) 
    {
        // File exists, load it
        bool missing_setting = false;

        if (GLOBAL_SETTING_LOAD.get("tcp-ip") == "") missing_setting = true; else global_settings.tcp_ip = GLOBAL_SETTING_LOAD.get("tcp-ip");
        if (GLOBAL_SETTING_LOAD.get("tcp-port") == "") missing_setting = true; else global_settings.tcp_port = stoi(GLOBAL_SETTING_LOAD.get("tcp-port"));
        if (GLOBAL_SETTING_LOAD.get("trace") == "") missing_setting = true; else global_settings.trace = (bool)stoi(GLOBAL_SETTING_LOAD.get("trace"));
        if (GLOBAL_SETTING_LOAD.get("trace-level") == "") missing_setting = true; else global_settings.trace_level = stoi(GLOBAL_SETTING_LOAD.get("trace-level"));

        if (missing_setting) writeIni();
    }
    else
    {
        // File does not exist, generate it.
        GLOBAL_SETTING_WRITE["tcp-ip"] = global_settings.tcp_ip;
        GLOBAL_SETTING_WRITE["tcp-port"] = std::to_string(global_settings.tcp_port);
        GLOBAL_SETTING_WRITE["trace"] = std::to_string(global_settings.trace);
        GLOBAL_SETTING_WRITE["trace-level"] = std::to_string(global_settings.trace_level);
        file.generate(ini);
    }
}

void writeIni() 
{
    GLOBAL_SETTING_WRITE["tcp-ip"] = global_settings.tcp_ip;
    GLOBAL_SETTING_WRITE["tcp-port"] = std::to_string(global_settings.tcp_port);
    GLOBAL_SETTING_WRITE["trace"] = std::to_string(global_settings.trace);
    GLOBAL_SETTING_WRITE["trace-level"] = std::to_string(global_settings.trace_level);
    file.write(ini);
    global_settings.has_changed = false;
}

//void addDeviceToList(canFrame)

