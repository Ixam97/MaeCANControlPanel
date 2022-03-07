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
 * Commit: [2022-03-07.1]
 */

#ifndef GUI_H_
#define GUI_H_

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "SDL.h"
#include "SDL_main.h"
#include "canframe.h"
#include <queue>
#include "globals.h"

enum CustomWindowFlags_
{
    DialogWindowFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize
};

class GUI
{
private:
    static inline SDL_Renderer* m_renderer = nullptr;
    static inline SDL_Window* m_window = nullptr;
    static inline ImFont* m_font = nullptr;
    static inline ImFont* m_bold_font = nullptr;
    static inline ImFont* m_consolas = nullptr;

    static inline SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);;

    static inline std::vector<canFrame> m_consoleVector;
    static inline std::queue<canFrame> m_frameOutQueue;

    static inline int m_x_res, m_y_res = 0;

    static inline bool m_exit = false;
    static inline float m_scaling = 1;

    static inline bool m_draw_settings = false;
    static inline bool m_draw_consoles = true;
    static inline bool m_draw_device_manager = true;
    static inline bool m_draw_debug_monitor = false;
    static inline bool m_draw_info = false;
    static inline bool m_draw_updater = false;

    static inline uint32_t m_update_uid;
    static inline uint16_t m_update_type;
    static inline std::string m_update_name;

    static inline ImVec2 m_main_size;
    static inline ImVec2 m_status_size;
    static inline ImVec2 m_stopgo_size;

    static void drawDebugMonitor();
    static void drawSettings();
    static void drawConsoles();
    static void drawDeviceManager();
    static void drawInfo();
    static void drawUpdateInfo();

    static void addFrameToQueue(canFrame _frame);
    static ImVec4 byteToColor(uint8_t _byte);
    static void customSytle(ImGuiStyle* dst = NULL);
    static void helpMarker(const char* desc);
    static void consoleDecoder(canFrame& _frame, bool& _easy_mode);

    static inline uint8_t stopData[8] = { 0,0,0,0,0,0,0,0 };
    static inline uint8_t goData[8] = { 0,0,0,0,1,0,0,0 };
    static inline canFrame m_stopFrame = canFrame(0x00000300,5,stopData);
    static inline canFrame m_goFrame = canFrame(0x00000300,5,goData);

public:

    static void GUISetup(int x_res, int y_res, const char* window_name);

    // Poll events on every loop
    static void poll(bool* _exit);

    // Begin new ImGui-Frame on every loop
    static void newFrame();

    // Draw the main window an ints sub windows
    static void drawMainWindow();

    // Render GUI at the end of every loop
    static void render();

    // Cleanup at program end
    static void cleanup();

    // Add a frame to the can monitor
    static void addFrameToConsoleVector(canFrame _frame);

    // Get a CAN frame from the internal output buffer
    static bool getFrame(canFrame& _frame);
};

#endif
