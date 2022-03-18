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
 * busmonitor.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-18.1]
 */

#include "busmonitor.h"
#include "guihelpers.h"
#include "vector"
#include <sstream>
#include <iomanip>

#define CMD_COMBO_STRING   u8"System\0Lok Discovery\0MFX Bind\0MFX Verify\0Lok Geschwindigkeit\0Lok Richtung\0Lok Funktion\0Read Config\0Write Config\0Zubehör Schalten\0Zubehör Konfig\0S88 Polling\0S88 Event\0SX1 Event\0Teilnehmer Ping\0Updateangebot\0Read Config Data\0Bootloader CAN\0Bootloader Schiene\0Statusdaten Konfiguration\0Data Query\0Config Data Stream\0Connect Data Stream\0"

uint8_t m_cmd_table[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x0B, 0x0C, 0x10, 0x11, 0x12, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x20, 0x21, 0x22 };

class CanFilter;

void removeFilterFromList(int _index);
void swapFilters(int _index_a, int _index_b);

std::vector<Globals::CanFrame> m_console_vector;
std::vector<CanFilter> m_filter_vector;

class CanFilter
{
private:
    int m_exclude = 0;
    uint8_t m_cmd_filter = 0;
    bool b_cmd_filter = false;
    int i_cmd_filter = 0;
    bool b_hash_filter = false;
    char s_hash_filter[5] = "";
    uint16_t m_hash_filter = 0;
    bool m_resp_filter = false;
public:
    bool isExcluding() { return (bool)m_exclude; }

    bool filter(Globals::CanFrame& _frame)
    {
        bool match = true;
        if (b_cmd_filter && _frame.cmd != m_cmd_filter)
            match = false;
        if (b_hash_filter && _frame.can_hash != m_hash_filter)
            match = false;
        if (b_cmd_filter && _frame.resp != (int)m_resp_filter)
            match = false;

        return match;
    }

    void filterSettingsWidget(int _index)
    {
        ImGui::Separator();
        ImGui::PushID(_index);
        ImGui::PushItemWidth(100 * Globals::ProgramStates::gui_scaling);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        if (ImGui::Button(u8"\u2715"))
            removeFilterFromList(_index);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ButtonDisablable(u8"\u25b2", (_index == 0)))
            swapFilters(_index - 1, _index);
        ImGui::SameLine();
        if (ButtonDisablable(u8"\u25bc", (_index == m_filter_vector.size() - 1)))
            swapFilters(_index + 1, _index);
        ImGui::SameLine();
        ImGui::Combo("##", &m_exclude, "Include\0Exclude\0");
        ImGui::SameLine();
        ImGui::Checkbox("Befehl", &b_cmd_filter);
        ImGui::SameLine();
        if (!b_cmd_filter) ImGui::BeginDisabled();
        ImGui::Combo("##cmdCombo", &i_cmd_filter, CMD_COMBO_STRING);
        m_cmd_filter = m_cmd_table[i_cmd_filter];
        ImGui::SameLine();
        ImGui::Checkbox("Resp.", &m_resp_filter);
        ImGui::SameLine();
        ImGui::Text("0x%02X", (m_cmd_filter << 1) | (int)m_resp_filter);
        if (!b_cmd_filter) ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::Checkbox("Hash", &b_hash_filter);
        ImGui::SameLine();
        if (!b_hash_filter) ImGui::BeginDisabled();
        ImGui::InputText("##hashText", s_hash_filter, 5, ImGuiInputTextFlags_CharsHexadecimal);
        if (!b_hash_filter) ImGui::EndDisabled();
        if (s_hash_filter[0]) m_hash_filter = stoi(std::string(s_hash_filter), nullptr, 16);
        ImGui::SameLine();
        ImGui::Text("0x%04X", m_hash_filter);
        ImGui::PopID();
        ImGui::PopItemWidth();
    }
};

void removeFilterFromList(int _index)
{
    if (_index < m_filter_vector.size())
    {
        if (_index == m_filter_vector.size() - 1)
        {
            m_filter_vector.pop_back();
            return;
        }
        for (size_t i = _index; i < m_filter_vector.size() - 1; i++)
        {
            m_filter_vector[i] = m_filter_vector[i + 1];
        }
        m_filter_vector.pop_back();
    }
}

void swapFilters(int _index_a, int _index_b)
{
    if (_index_a >= 0 && _index_b >= 0 && _index_a < m_filter_vector.size() && _index_b < m_filter_vector.size())
    {
        std::swap(m_filter_vector[_index_a], m_filter_vector[_index_b]);
    }
}

namespace BusMonitor
{
    std::string getLoco(uint8_t* data) 
    {
        uint16_t locID = (data[2] << 8) | data[3];
        std::stringstream ss_loco;
        char prot[32];

        memset(prot, 0, sizeof(prot));

        if (locID <= 0x03ff)
            ss_loco << "mm-" << locID;
        else if (locID >= 0x4000 && locID < 0xC000)
            ss_loco << "mfx-" << locID - 0x4000;
        else if (locID >= 0xC000)
            ss_loco << "dcc-" << locID - 0xC000;
        else
            ss_loco << "unbekannt-" << locID;
        return ss_loco.str();
    }

    void consoleDecoder(Globals::CanFrame& _frame, bool decode)
    {
        std::stringstream ss_frame_stream;
        ss_frame_stream << "0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << _frame.id << " [" << std::dec << (int)_frame.dlc << "] ";

        for (int i = 0; i < 8; i++)
        {
            if (i < _frame.dlc)
                ss_frame_stream << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)_frame.data[i] << " ";
            else
                ss_frame_stream << "   ";
        }

        ImGui::Text(ss_frame_stream.str().c_str());

        if (decode)
        {
            uint32_t frame_uid = Globals::getUidFromData(_frame.data);

            if (_frame.resp) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 1, 1));
            else ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));

            switch (_frame.cmd)
            {
            case CMD_SYS:
            {
                ImGui::SameLine();
                ImGui::Text("System:");
                ImGui::SameLine();
                if (_frame.dlc == 4)
                {
                    ImGui::Text("Stopp / Go - Abfrage");
                }
                else if (_frame.dlc >= 5)
                {
                    switch (_frame.data[4])
                    {
                    case SYS_STOP:
                        if (frame_uid)
                            ImGui::Text("UID 0x%08X", frame_uid);
                        else
                            ImGui::Text("alle");
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
                        ImGui::Text("STOP");
                        ImGui::PopStyleColor();
                        break;
                    case SYS_GO:
                        if (frame_uid)
                            ImGui::Text("UID 0x%08X", frame_uid);
                        else
                            ImGui::Text("alle");
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0, 1.0, 0.0, 1.0));
                        ImGui::Text("Go");
                        ImGui::PopStyleColor();
                        break;
                    case SYS_HALT:
                        ImGui::Text("Halt");
                        break;
                    case SYS_LOCO_HALT:
                        if (frame_uid)
                            ImGui::Text("Lok 0x%08X Nothalt", frame_uid);
                        else
                            ImGui::Text("Alle Loks Nothalt");
                        break;
                    case SYS_LOCO_CYCLE_END:
                        ImGui::Text("Lok %s Zyklus Ende", getLoco(_frame.data).c_str());
                        break;
                    case SYS_LOCO_DATAPROT:
                        ImGui::Text("Lok %s Gleisprotokoll: %d", getLoco(_frame.data).c_str(), _frame.data[5]);
                        break;
                    case SYS_ACC_TIME:
                        ImGui::Text(u8"Schaltzeit Zubehör UID 0x%08X Zeit %d", frame_uid, (_frame.data[5] << 8) | _frame.data[6]);
                        break;
                    case SYS_MFX_FAST_READ:
                        ImGui::Text("Fast Read MFX: UID 0x%08X SID %d", frame_uid, (_frame.data[5] << 8) | _frame.data[6]);
                        break;
                    case SYS_TRACKPROT:
                        ImGui::Text("Gleisprotokoll freischeilten:");
                        if (_frame.data[5] & 1) { ImGui::SameLine(); ImGui::Text("MM2"); }
                        if (_frame.data[5] & 2) { ImGui::SameLine(); ImGui::Text("MFX"); }
                        if (_frame.data[5] & 4) { ImGui::SameLine(); ImGui::Text("DCC"); }
                        if (_frame.data[5] & 8) { ImGui::SameLine(); ImGui::Text("SX1"); }
                        if (_frame.data[5] & 16) { ImGui::SameLine(); ImGui::Text("SX2"); }
                        break;
                    case SYS_MFX_REG_RESET:
                        ImGui::Text(u8"Neuanmeldezähler setzen UID 0x%08X Zähler 0x%04X", frame_uid, (_frame.data[5] << 8) | _frame.data[6]);
                        break;
                    case SYS_OVERLOAD:
                        ImGui::Text(u8"Überlast UID 0x%08X Kanal 0x%04X", frame_uid, _frame.data[5]);
                        break;
                    case SYS_STAT:
                        if (_frame.dlc == 6)
                            ImGui::Text("Statusabfrage UID 0x%08X Kanal 0x%02X", frame_uid, _frame.data[5]);
                        else if (_frame.dlc == 7)
                        {
                            ImGui::Text("Konfiguration UID 0x%08X Kanal 0x%02X", frame_uid, _frame.data[5]);
                            if (_frame.data[6]) { ImGui::SameLine(); ImGui::Text(u8"gültig(%d)", _frame.data[6]); }
                            else { ImGui::SameLine(); ImGui::Text(u8"ungültig(%d)", _frame.data[6]); }
                        }
                        else if (_frame.dlc == 8)
                        {
                            if (_frame.resp == 1) ImGui::Text("Statusabfrage UID 0x%08X Kanal 0x%02X Messwert 0x%04X", frame_uid, _frame.data[5], (_frame.data[6] << 8) | _frame.data[7]);
                            else ImGui::Text("Konfiguration UID 0x%08X Kanal 0x%02X Konfigurationswert 0x%04X", frame_uid, _frame.data[5], (_frame.data[6] << 8) | _frame.data[7]);
                        }
                        break;
                    case SYS_ID:
                        if (_frame.dlc == 5) ImGui::Text(u8"System: Gerätekennung UID 0x%08X", frame_uid);
                        else ImGui::Text(u8"System: Gerätekennung UID 0x%08X ist 0x%04X", frame_uid, (_frame.data[5] << 8) | _frame.data[6]);
                        break;
                    case SYS_TIME:
                        ImGui::Text("Systemzeit UID 0x%08X Stunde %d Minute %d Faktor %d", frame_uid, _frame.data[5], _frame.data[6], _frame.data[7]);
                        break;
                    case SYS_MFX_SEEK:
                        ImGui::Text("mfx Seek 0x%08X", frame_uid);
                        break;
                    case SYS_SYSTEM_RESET:
                        ImGui::Text("System Reset UID 0x%08X Ziel 0x%02X", frame_uid, _frame.data[5]);
                        break;
                    default:
                        ImGui::Text("Unbekannter Befehl: 0x%02X", _frame.data[4]);
                        break;
                    }
                }
                break;
            }
            case CMD_LOCO_DIS:
            {
                ImGui::SameLine();
                if (_frame.dlc == 0)
                    ImGui::Text("Lok Discovery - Erkennen alle Protokolle");
                if (_frame.dlc == 1)
                    ImGui::Text("Lok Discovery - Protokoll Kennung 0x%02X", _frame.data[0]);
                if (_frame.dlc == 5)
                    ImGui::Text("Lok Discovery - 0x%04X Protokoll Kennung 0x%02X", frame_uid, _frame.data[4]);
                if (_frame.dlc == 6)
                    ImGui::Text("Lok Discovery - 0x%04X Range %d ASK %d", frame_uid, _frame.data[4], _frame.data[5]);
                break;
            }
            case CMD_MFX_BIND:
            {
                ImGui::SameLine();
                if (_frame.dlc == 6)
                    ImGui::Text("MFX Bind: MFX UID 0x%08X MFX SID %d", frame_uid, (_frame.data[4] << 8) | _frame.data[5]);
                break;
            }
            case CMD_MFX_VERIFY:
            {
                ImGui::SameLine();
                if (_frame.dlc == 2) {
                    if ((uint16_t)(frame_uid >> 16) == 0x00ff)
                        ImGui::Text("CdB: Reset");
                    else
                        ImGui::Text("CdB: unbekannt 0x%04x", (uint16_t)(frame_uid >> 16));
                }
                if (_frame.dlc == 4) {
                    //cdb_extension_set_grd(frame);
                }
                if (_frame.dlc == 6)
                    ImGui::Text("MFX Verify: MFX UID 0x%08X MFX SID %d", frame_uid, (_frame.data[4] << 8) | _frame.data[5]);
                if (_frame.dlc == 7)
                    ImGui::Text(u8"MFX Verify: MFX UID 0x%08X MFX SID %d ASK-Verhältnis %d", frame_uid, (_frame.data[4] << 8) | _frame.data[5], _frame.data[6]);
                break;
            }
            case CMD_LOCO_SPEED:
            {
                ImGui::SameLine();
                if (_frame.dlc == 4)
                    ImGui::Text("Lok %s Abfrage Fahrstufe", getLoco(_frame.data).c_str());
                else if (_frame.dlc == 6)
                    ImGui::Text("Lok %s Geschwindigkeit: %3.1f", getLoco(_frame.data).c_str(), (float)((_frame.data[4] << 8) | _frame.data[5]) / 10);
                break;
            }
            case CMD_LOCO_DIR:
            {
                break;
            }
            case CMD_LOCO_FN:
            {
                break;
            }
            case CMD_READ_CONF:
            {
                break;
            }
            case CMD_WRITE_CONF:
            {
                break;
            }
            case CMD_SWITCH_ACC:
            {
                break;
            }
            case CMD_CONFIG_ACC:
            {
                break;
            }
            case CMD_S88_POLL:
            {
                break;
            }
            case CMD_S88_EVENT:
            {
                break;
            }
            case CMD_SX1_EVENT:
            {
                break;
            }
            case CMD_PING:
            {
                ImGui::SameLine();
                if (_frame.resp)
                {
                    ImGui::Text("Ping Antwort von ");
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
                            ImGui::Text(u8"MäCAN Busankoppler");
                        break;
                    case 0x0052:
                        if ((frame_uid & 0xffff0000) == 0x4d430000)
                            ImGui::Text(u8"MäCAN MP5x16");
                        break;
                    case 0x0053:
                        if ((frame_uid & 0xffff0000) == 0x4d430000)
                            ImGui::Text(u8"MäCAN Dx32");
                        else
                            ImGui::Text("Cg Servo");
                        break;
                    case 0x0054:
                        ImGui::Text("Cg Rückmelder");
                        break;
                    case 0x1234:
                        ImGui::Text(u8"MäCAN-Weichendecoder");
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
                    ImGui::Text("UID 0x%08X, Software Version %d.%d", frame_uid, _frame.data[4], _frame.data[5]);
                    break;
                }
                else
                {
                    ImGui::Text("Ping Anfrage");
                }
                break;
            }
            case CMD_UPDATE_OFFER:
            {
                break;
            }
            case CMD_R_CONFIG_DATA:
            {
                break;
            }
            case CMD_BOOTLOADER:
            {
                break;
            }
            case CMD_BL_TRACK:
            {
                break;
            }
            case CMD_CONFIG:
            {
                break;
            }
            case CMD_DATA_QUERY:
            {
                break;
            }
            case CMD_DATA_STREAM:
            {
                break;
            }
            case CMD_CONNECT_STREAM:
            {
                break;
            }

            default:
                break;

            }

            ImGui::PopStyleColor();
        }
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
            static bool b_auto_scroll = true;
            static bool b_decode = true;
            static int max_console_lines = 200;

            static bool b_jump_to_bottom = false;
            static size_t vector_size = 0;
            bool copy_to_clipboard = false;

            while (m_console_vector.size() > max_console_lines)
            {
                for (size_t i = 0; i < m_console_vector.size() - 1; i++)
                    m_console_vector[i] = m_console_vector[i + 1];
                m_console_vector.pop_back();
            }

            if (vector_size != m_console_vector.size()) {
                vector_size = m_console_vector.size();
                if (b_auto_scroll) 
                    b_jump_to_bottom = true;
            }

            if (ImGui::BeginPopup("Optionen")) {
                ImGui::Checkbox("Automatisches Scrollen", &b_auto_scroll);
                ImGui::Checkbox(u8"Aufschlüsseln", &b_decode);
                ImGui::InputInt("Max. Zeilen", &max_console_lines, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue);
                if (max_console_lines < 0) 
                    max_console_lines = 0;

                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Filter")) {
                if (ImGui::Button(u8"Hinzufügen")) m_filter_vector.push_back(CanFilter());
                ImGui::SameLine();
                if (ButtonDisablable("Entfernen", !(m_filter_vector.size() > 0)) && m_filter_vector.size() > 0) m_filter_vector.pop_back();
                int tmp_index = 0;
                for (auto& filter_obj : m_filter_vector)
                {
                    filter_obj.filterSettingsWidget(tmp_index);
                    tmp_index++;
                }
                ImGui::EndPopup();
            }
            std::stringstream ss_button_label;

            ss_button_label << "Filter (" << m_filter_vector.size() << ")";

            if (ImGui::SmallButton("Optionen")) ImGui::OpenPopup("Optionen");
            ImGui::SameLine();
            if (ImGui::SmallButton(ss_button_label.str().c_str())) ImGui::OpenPopup("Filter");
            ImGui::SameLine();
            if (ImGui::SmallButton("Leeren")) m_console_vector.resize(0);
            ImGui::SameLine();
            copy_to_clipboard = ImGui::SmallButton("In Zwischenablage kopieren");

            ImGui::Separator();

            ImGui::BeginChild("Scrolling Region");
            ImGui::PushFont((ImFont*)console_font);

            if (copy_to_clipboard) ImGui::LogToClipboard();

            for (Globals::CanFrame& n : m_console_vector)
                consoleDecoder(n, b_decode);

            if (copy_to_clipboard) 
                ImGui::LogFinish();

            if (b_jump_to_bottom || (b_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
                ImGui::SetScrollHereY(1.0f);
            b_jump_to_bottom = false;

            ImGui::PopFont();
            ImGui::EndChild();

            ImGui::End();
        }

    }

    void addFrame(Globals::CanFrame& _frame)
    {
        if (m_filter_vector.size() == 0)
            m_console_vector.push_back(_frame);
        else
        {
            bool b_filter = false;
            for (auto& n_filter : m_filter_vector)
            {
                if (n_filter.filter(_frame))
                    b_filter = n_filter.isExcluding();
            }
            if (!b_filter)
                m_console_vector.push_back(_frame);
        }
    }

    bool getFrame(Globals::CanFrame& _frame)
    {
        return false;
    }
}
