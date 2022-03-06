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
 * consoledecoder.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-06.1]
 */

 /*
 CAN monitor decoder
 derived from https://github.com/GBert/railroad/blob/master/can2udp/src/can-monitor.c
 */

#pragma once
#include "gui.h"
#include <sstream>
#include <iomanip>

void GUI::consoleDecoder(canFrame& _frame, bool& _easy_mode)
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

    uint32_t frame_uid = getUidFromData(_frame.data);

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
