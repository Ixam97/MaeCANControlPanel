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
 * Commit: [2022-03-10.1]
 */

#include "feedbackmonitor.h"
#include "imgui.h"

int m_starting_adress = 1;
int m_bus_id = 0;

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
			ImGui::InputInt("Buskennung", &m_bus_id);

			if (m_starting_adress < 1) m_starting_adress = 1;
			if (m_bus_id < 0) m_bus_id = 0;

			ImGui::Separator();

			ImGui::Button("         "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  ");
			ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  "); ImGui::SameLine(); ImGui::Button("  ");

			ImGui::End();
		}
	}

	void addFrame(Interface::CanFrame& _frame)
	{
		// Process Frame
	}

	bool getFrame(Interface::CanFrame& _frame)
	{
		// Return frames if needed
		return false;
	}
}
