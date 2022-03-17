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
 * Commit: [2022-03-17.1]
 */

#include "feedbackmonitor.h"
#include "imgui.h"

int m_starting_adress = 1;
int m_bus_id = 0;
bool m_simulate_feedback = false;

bool m_states[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };

void feedbackButton(bool _state)
{
	if (_state)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	}
	ImGui::Button("  ");
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
			ImGui::InputInt("Anfangsadresse", &m_starting_adress, 16, 16);
			ImGui::BeginDisabled();
			ImGui::InputInt("Buskennung", &m_bus_id);

			ImGui::Button(u8"Rückmelder einlesen");
			ImGui::EndDisabled();
			ImGui::SameLine();
			if (ImGui::Button(u8"Zurücksetzen"))
			{
				for (int i = 0; i < 16; i++)
					m_states[i] = false;
			}
			ImGui::BeginDisabled();
			ImGui::Checkbox(u8"Rückmeldersimulation", &m_simulate_feedback);
			ImGui::EndDisabled();

			if (m_starting_adress < 1) m_starting_adress = 1;
			if (m_bus_id < 0) m_bus_id = 0;

			ImGui::Separator();

			for (int i = 0; i < 16; i++)
			{
				feedbackButton(m_states[i]);
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
