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
 * busmonitor.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-10.1]
 */

#include "busmonitor.h"
#include "imgui.h"
#include "vector"
#include <sstream>
#include <iomanip>

std::vector<Interface::CanFrame> m_console_vector;


namespace BusMonitor
{
    void consoleDecoder(Interface::CanFrame& _frame)
    {
        //if (!_easy_mode) ImGui::Text("0x%08X [%d] %02X %02X %02X %02X %02X %02X %02X %02X", _frame.id, _frame.dlc, _frame.data[0], _frame.data[1], _frame.data[2], _frame.data[3], _frame.data[4], _frame.data[5], _frame.data[6], _frame.data[7]);
        //else ImGui::Text("CMD: 0x%02X RESP.: %d HASH: %04X DLC: %d DATA: %02X %02X %02X %02X %02X %02X %02X %02X", _frame.cmd, _frame.resp, _frame.can_hash, _frame.dlc, _frame.data[0], _frame.data[1], _frame.data[2], _frame.data[3], _frame.data[4], _frame.data[5], _frame.data[6], _frame.data[7]);

        std::stringstream ss_frame_stream;
        ss_frame_stream << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << _frame.id << " [" << std::dec << (int)_frame.dlc << "] ";

        for (int i = 0; i < 8; i++)
        {
            if (i < _frame.dlc)
            {
                ss_frame_stream << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)_frame.data[i] << " ";
            }
            else
            {
                ss_frame_stream << "   ";
            }
        }

        ImGui::Text(ss_frame_stream.str().c_str());

        uint32_t frame_uid = Interface::getUidFromData(_frame.data);

        if (_frame.resp) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 1, 1));
        else ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));

        switch (_frame.cmd)
        {
        case SYS_CMD:
            ImGui::SameLine();
            ImGui::Text(" System:");
            if (_frame.dlc == 5)
            {
                ImGui::SameLine();
                if (frame_uid)
                {
                    ImGui::Text("UID 0x%08X", frame_uid);
                }
                else
                {
                    ImGui::Text("alle");
                }
                if (_frame.data[4] == SYS_STOP)
                {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
                    ImGui::Text("STOP");
                    ImGui::PopStyleColor();
                }
                else if (_frame.data[4] == SYS_GO)
                {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0, 1.0, 0.0, 1.0));
                    ImGui::Text("Go");
                    ImGui::PopStyleColor();
                }
            }

            break;
        case CMD_PING:
            ImGui::SameLine();
            if (_frame.resp)
            {
                ImGui::Text(" Ping Antwort von ");
                ImGui::SameLine();
                uint16_t _id = (_frame.data[6] << 8) | _frame.data[7];
                switch (_id)
                {
                case 0x0000:
                    if ((frame_uid & 0xff000000) == 0x42000000)
                        ImGui::Text("Booster (6017x)");
                    else
                        ImGui::Text("GFP");
                    break;
                case 0x0010:
                case 0x0011:
                    ImGui::Text("Gleisbox");
                    break;
                case 0x0020:
                    ImGui::Text("Connect6021");
                    break;
                case 0x0030:
                case 0x0031:
                case 0x0032:
                case 0x0033:
                    ImGui::Text("MS2");
                    break;
                case 0x0040:
                    if ((frame_uid & 0xFFF00000) == 0x53300000)
                        ImGui::Text("LinkS88");
                    else
                        ImGui::Text("S88 Gateway");
                    break;
                case 0x0051:
                    if ((frame_uid & 0xffff0000) == 0x4d430000)
                        ImGui::Text(u8"M‰CAN Busankoppler");
                    break;
                case 0x0052:
                    if ((frame_uid & 0xffff0000) == 0x4d430000)
                        ImGui::Text(u8"M‰CAN MP5x16");
                    break;
                case 0x0053:
                    if ((frame_uid & 0xffff0000) == 0x4d430000)
                        ImGui::Text(u8"M‰CAN Dx32");
                    else
                        ImGui::Text("Cg Servo");
                    break;
                case 0x0054:
                    ImGui::Text("Cg R¸ckmelder");
                    break;
                case 0x1234:
                    ImGui::Text(u8"M‰CAN-Weichendecoder");
                    break;
                case 0xEEEE:
                    ImGui::Text("CS2 Software");
                    break;
                case 0xFFFF:
                    ImGui::Text("CS2-GUI (Master)");
                    break;
                default:
                    ImGui::Text("unbekannt");
                    break;
                }
                ImGui::SameLine();
                ImGui::Text(" UID 0x%08X, Software Version %d.%d", frame_uid, _frame.data[4], _frame.data[5]);
                break;
            }
            else
            {
                ImGui::Text(" Ping Anfrage");
            }
            break;
        default:
            break;

        }

        ImGui::PopStyleColor();
    }

    void draw(void* console_font)
    {
        if (b_draw)
        {
            if (!ImGui::Begin("CAN-Monitor", &b_draw, ImGuiWindowFlags_NoCollapse))
            {
                ImGui::End();
                return;
            }
            static bool AutoScroll = true;
            // static bool EasyMode = false;
            static int max_console_lines = 200;

            static bool JumpToBottom = false;
            static size_t vector_size = 0;
            bool copy_to_clipboard = false;
            static int cmd_filter = 0;
            static bool b_cmd_filter = false;

            while (m_console_vector.size() > max_console_lines)
            {
                for (size_t i = 0; i < m_console_vector.size() - 1; i++)
                {
                    m_console_vector[i] = m_console_vector[i + 1];
                }
                m_console_vector.pop_back();
            }

            if (vector_size != m_console_vector.size()) {
                vector_size = m_console_vector.size();
                if (AutoScroll) JumpToBottom = true;
            }

            if (ImGui::BeginPopup("Optionen")) {
                ImGui::Checkbox("Automatisches Scrollen", &AutoScroll);
                // ImGui::Checkbox("Vereinfachter Modus", &EasyMode);
                ImGui::InputInt("Max. Zeilen", &max_console_lines, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue);
                if (max_console_lines < 0) max_console_lines = 0;

                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Filter")) {
                ImGui::Checkbox("Befehlsfilter", &b_cmd_filter);
                ImGui::InputInt("##", &cmd_filter, 1, 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::EndPopup();
            }

            if (ImGui::SmallButton("Optionen")) ImGui::OpenPopup("Optionen");
            ImGui::SameLine();
            if (ImGui::SmallButton("Filter")) ImGui::OpenPopup("Filter");
            ImGui::SameLine();
            if (ImGui::SmallButton("Leeren")) m_console_vector.resize(0);
            ImGui::SameLine();
            copy_to_clipboard = ImGui::SmallButton("In Zwischenablage kopieren");

            ImGui::Separator();

            //const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
            ImGui::BeginChild("Scrolling Region");
            ImGui::PushFont((ImFont*)console_font);

            if (copy_to_clipboard) ImGui::LogToClipboard();

            for (Interface::CanFrame& n : m_console_vector)
            {
                if (b_cmd_filter)
                {
                    if (n.cmd == cmd_filter)
                        consoleDecoder(n);
                }
                else
                    consoleDecoder(n);
            }

            if (copy_to_clipboard) ImGui::LogFinish();

            if (JumpToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
                ImGui::SetScrollHereY(1.0f);
            JumpToBottom = false;

            ImGui::PopFont();
            ImGui::EndChild();

            ImGui::End();
        }

    }

    void addFrame(Interface::CanFrame& _frame)
    {
        m_console_vector.push_back(_frame);
    }

    bool getFrame(Interface::CanFrame& _frame)
    {
        return false;
    }
}
