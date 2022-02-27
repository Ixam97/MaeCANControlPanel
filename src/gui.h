#ifndef GUI_H_
#define GUI_H_

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "SDL.h"
#include "SDL_main.h"
//#include "can.h"
#include <stdio.h>
#include <vector>

#include "globals.h"


class GUI
{
private:
    SDL_Renderer* m_renderer;
    SDL_Window* m_window;
    ImFont* m_font;
    ImFont* m_consolas;

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);;

    std::vector<canFrame> m_consoleVector;

    int m_x_res, m_y_res;

    bool m_exit = false;
    float m_scaling = 1;

    bool m_draw_settings = false;
    bool m_draw_consoles = true;
    bool m_draw_device_manager = true;
    bool m_draw_debug_monitor = false;

    ImVec2 m_main_size;
    ImVec2 m_status_size;
    ImVec2 m_stopgo_size;

    void drawDebugMonitor();
    void drawSettings();
    void drawConsoles();
    void drawDeviceManager();
    ImVec4 byteToColor(uint8_t _byte);
    void customSytle(ImGuiStyle* dst = NULL);
    void helpMarker(const char* desc);

    CAN* m_can;

    uint8_t stopData[8] = { 0,0,0,0,0,0,0,0 };
    uint8_t goData[8] = { 0,0,0,0,1,0,0,0 };
    canFrame m_stopFrame = canFrame(0x00000300,5,stopData);
    canFrame m_goFrame = canFrame(0x00000300,5,goData);

public:

    GUI(int x_res, int y_res, const char* window_name, CAN* _can, float scaling = 1);
    GUI();

    void setScaling(float _scaling);

    // Poll events on every loop
    void poll(bool* _exit);

    // Begin new ImGui-Frame on every loop
    void newFrame();

    void drawMainWindow();

    // Render GUI at the end of every loop
    void render();

    // Cleanup at program end
    void cleanup();

    void addFrameToConsoleVector(canFrame _frame);
};

#endif