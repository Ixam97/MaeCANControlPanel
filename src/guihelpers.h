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
 * guihelpers.h
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-10.1]
 */

#pragma once
#include "imgui.h"

bool ButtonDisablable(const char* _label, bool _disable = false);

void helpMarker(const char* desc);

ImVec4 byteToColor(unsigned short _byte);

enum CustomWindowFlags_
{
    DialogWindowFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize
};
