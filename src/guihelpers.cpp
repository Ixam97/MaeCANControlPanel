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
 * guihelpers.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-10.1]
 */

#include "guihelpers.h"


bool ButtonDisablable(const char* _label, bool _disable)
{
    bool b_return;
    if (_disable) ImGui::BeginDisabled();
    b_return = ImGui::Button(_label);
    if (_disable) ImGui::EndDisabled();
    return b_return;

}

void helpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

ImVec4 byteToColor(unsigned short _byte)
{
    return ImVec4((float)((_byte & 0xc0) >> 6) / 0b11, (float)((_byte & 0x30) >> 4) / 0b11, (float)((_byte & 0x0c) >> 2) / 0b11, 1.00);
}
