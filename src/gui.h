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
 * gui.h
 * (c)2022 Maximilian Goldschmidt
 */

#ifndef GUI_H_
#define GUI_H_

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "SDL.h"
#include "SDL_main.h"
#include "canframe.h"
#include <windows.h>
#include <queue>
#include "globals.h"


class GUI
{
private:
    SDL_Renderer* m_renderer = nullptr;
    SDL_Window* m_window = nullptr;
    ImFont* m_font = nullptr;
    ImFont* m_consolas = nullptr;

    enum CustomWindowFlags_
    {
        DialogWindowFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize
    };

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);;

    std::vector<canFrame> m_consoleVector;
    std::queue<canFrame> m_frameOutQueue;

    int m_x_res, m_y_res = 0;

    bool m_exit = false;
    float m_scaling = 1;

    bool m_draw_settings = false;
    bool m_draw_consoles = true;
    bool m_draw_device_manager = true;
    bool m_draw_debug_monitor = false;
    bool m_draw_info = false;

    ImVec2 m_main_size;
    ImVec2 m_status_size;
    ImVec2 m_stopgo_size;

    void drawDebugMonitor();
    void drawSettings();
    void drawConsoles();
    void drawDeviceManager();
    void drawInfo();
    void addFrameToQueue(canFrame _frame);
    ImVec4 byteToColor(uint8_t _byte);
    void customSytle(ImGuiStyle* dst = NULL);
    void helpMarker(const char* desc);
    void consoleDecoder(canFrame& _frame, bool& _easy_mode);

    //CAN* m_can = nullptr;

    uint8_t stopData[8] = { 0,0,0,0,0,0,0,0 };
    uint8_t goData[8] = { 0,0,0,0,1,0,0,0 };
    canFrame m_stopFrame = canFrame(0x00000300,5,stopData);
    canFrame m_goFrame = canFrame(0x00000300,5,goData);

public:

    GUI(int x_res, int y_res, const char* window_name);

    // Poll events on every loop
    void poll(bool* _exit);

    // Begin new ImGui-Frame on every loop
    void newFrame();

    // Draw the main window an ints sub windows
    void drawMainWindow();

    // Render GUI at the end of every loop
    void render();

    // Cleanup at program end
    void cleanup();

    // Add a frame to the can monitor
    void addFrameToConsoleVector(canFrame _frame);

    // Get a CAN frame from the internal output buffer
    bool getFrame(canFrame& _frame);
};

#endif