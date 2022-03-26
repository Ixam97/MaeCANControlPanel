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
 * feedbackmonitor.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-27.1]
 */

#include "feedbackmonitor.h"
#include "guihelpers.h"

int m_starting_address = 1;
int m_bus_id = 0;
bool m_simulate_feedback = false;

bool m_states[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };

void feedbackButton(bool _state, int _index)
{
	if (_state)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	}
	ImGui::Button(std::to_string(_index).c_str(), ImVec2(30.0f * Globals::ProgramStates::gui_scaling, 30.0f * Globals::ProgramStates::gui_scaling));
	if (_state) ImGui::PopStyleColor(2);
}

namespace FeedbackMonitor {
	void draw()
	{
		if (b_draw)
		{
			if (!ImGui::Begin(u8"Rückmeldemonitor", &b_draw, ImGuiWindowFlags_NoCollapse))
			{
				ImGui::End();
				return;
			}
			ImGui::Button(u8"Rückmelder einlesen");
			ImGui::SameLine();
			if (ImGui::Button(u8"Zurücksetzen"))
			{
				for (int i = 0; i < 16; i++)
					m_states[i] = false;
			}
			ImGui::SameLine();
			ImGui::Checkbox(u8"Rückmeldersimulation", &m_simulate_feedback);
			// ImGui::InputInt("Anfangsadresse", &m_starting_adress, 16, 16);
			ImGui::Text("Kontakte:");
			ImGui::SameLine();
			if (ButtonDisablable(u8"\u25c0##adr", (m_starting_address <= 1)))
			{
				m_starting_address -= 16;
				if (m_starting_address < 1)
					m_starting_address = 1;
			}
			ImGui::SameLine();
			ImGui::Text("%03d - %03d", m_starting_address, m_starting_address + 15);
			ImGui::SameLine();
			if (ButtonDisablable(u8"\u25b6##adr"))
				m_starting_address += 16;

			ImGui::Text("Buskennung:");
			ImGui::SameLine();
			if (ButtonDisablable(u8"\u25c0##busID", (m_bus_id <= 0)))
			{
				m_bus_id -= 1;
				if (m_bus_id < 0)
					m_bus_id = 0;
			}
			ImGui::SameLine();
			ImGui::InputText("##busID", (char*)std::to_string(m_bus_id).c_str(), std::to_string(m_bus_id).size() + 1, ImGuiInputTextFlags_ReadOnly);
			ImGui::Text("%03d", m_bus_id);
			ImGui::SameLine();
			if (ButtonDisablable(u8"\u25b6##busID"))
				m_bus_id += 1;

			ImGui::Separator();

			for (int i = 0; i < 16; i++)
			{
				feedbackButton(m_states[i], i + m_starting_address);
				if (i < 15) ImGui::SameLine();
			}
			
			ImGui::End();
		}
	}

	void addFrame(Globals::CanFrame& _frame)
	{
		if (_frame.cmd == CMD_S88_EVENT && _frame.resp == 1)
		{
			for (int i = 1; i <= 16; i++) 
			{
				if (i == _frame.data[3])
					m_states[i-1] = _frame.data[5];
			}
		}
	}

	bool getFrame(Globals::CanFrame& _frame)
	{
		// Return frames if needed
		return false;
	}
}
