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
 * gui.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-18.1]
 */

#include "gui.h"
#include "guihelpers.h"
#include "devicemanager.h"
#include "feedbackmonitor.h"
#include "busmonitor.h"
#include "imgui.h"

#ifdef GLFW_GL3
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include "GLFW/glfw3.h" // Will drag system OpenGL headers
#else
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "SDL.h"
#include "SDL_main.h"
#endif

#include <sstream>
#include <queue>
#include <windows.h>
#include <shellapi.h>

#ifndef GLFW_GL3
#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif
#endif

namespace GUI
{
#ifdef GLFW_GL3
    GLFWwindow* m_window = nullptr;
#else
    SDL_Renderer* m_renderer = nullptr;
    SDL_Window* m_window = nullptr;
#endif
    ImFont* m_font_fallback = nullptr;
    ImFont* m_font = nullptr;
    ImFont* m_bold_font = nullptr;
    ImFont* m_consolas = nullptr;

    std::queue<Globals::CanFrame> m_frameOutQueue;

    int m_x_res, m_y_res = 0;

    bool m_exit = false;
    float m_scaling = 1;

    bool m_draw_settings = false;
    bool m_draw_info = false;

    ImVec2 m_main_size;
    ImVec2 m_status_size;
    ImVec2 m_stopgo_size;

    void drawModules();
    void addModuleMenuItems();

    void drawSettings();
    void drawInfo();

    void poll(bool& _exit);
    void newFrame();
    void render();

    void addFrameToQueue(Globals::CanFrame _frame);
    void customSytle(ImGuiStyle* dst = NULL);

    uint8_t stopData[8] = { 0,0,0,0,0,0,0,0 };
    uint8_t goData[8] = { 0,0,0,0,1,0,0,0 };
    Globals::CanFrame m_stopFrame(0x00000300, 5, stopData);
    Globals::CanFrame m_goFrame(0x00000300, 5, goData);

    // --------------------------- //
    // Add new module windows here //
    // --------------------------- //
    void drawModules()
    {
        DeviceManager::draw((void*)m_bold_font, m_scaling);
        FeedbackMonitor::draw();
        BusMonitor::draw((void*)m_consolas);
    }

    // ------------------------------------------ //
    // Add new menu items for module windows here //
    // ------------------------------------------ //
    void addModuleMenuItems()
    {
        ImGui::MenuItem("Gerätemanager", "", &DeviceManager::b_draw);
        ImGui::MenuItem("CAN-Monitor", "", &BusMonitor::b_draw);
        ImGui::MenuItem("Rückmeldemonitor", "", &FeedbackMonitor::b_draw);
        ImGui::BeginDisabled();
        ImGui::MenuItem("CV-Editor");
        ImGui::MenuItem("Magnetartikeltester");
        ImGui::EndDisabled();
    }

#ifndef GLFW_GL3
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#else
    static void glfw_error_callback(int error, const char* description)
    {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }
#endif

    void setup(int x_res, int y_res, const char* window_name) {

        SetProcessDPIAware();
        m_scaling = (float)GetDeviceCaps(GetDC(NULL), LOGPIXELSX) / 96;
        Globals::ProgramStates::gui_scaling = m_scaling;

        m_x_res = x_res;
        m_y_res = y_res;

#ifdef GLFW_GL3
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return;
        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char* glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
        // GL 3.2 + GLSL 150
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

        // Create window with graphics context
        m_window = glfwCreateWindow(m_x_res, m_y_res, window_name, NULL, NULL);
        if (m_window == NULL)
            return;
        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1); // Enable vsync
#else
        // Setup SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        {
            printf("Error: %s\n", SDL_GetError());
            //return false;
        }
        m_window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_x_res, m_y_res, window_flags);

        // Setup SDL_Renderer instance
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (m_renderer == NULL)
        {
            SDL_Log("Error creating SDL_Renderer!");
            //return false;
        }
#endif
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#ifdef GLFW_GL3
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
#endif
        io.IniFilename = "MaeCANControlPanelGUI.ini";

        // Setup Dear ImGui style
        customSytle();

#ifdef GLFW_GL3
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
#else
        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
        ImGui_ImplSDLRenderer_Init(m_renderer);
#endif

        const ImWchar font_ranges[] = { 0x25b2, 0x25b2, // Up arrow
                                        0x25bc, 0x25bc, // Down arrow
                                        0x2715, 0x2715, // Cross
                                        0 };
        ImFontConfig font_config;
        font_config.MergeMode = true;

        m_font_fallback = ImGui::GetIO().Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\seguisym.ttf", floor(18.0f * m_scaling));
        m_font = ImGui::GetIO().Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\seguisym.ttf", floor(18.0f * m_scaling), &font_config, font_ranges);
        m_bold_font = ImGui::GetIO().Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\seguisym.ttf", floor(24.0f * m_scaling));
        m_consolas = ImGui::GetIO().Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\consola.ttf", floor(14.0f * m_scaling));

        m_status_size.y = floor(30 * m_scaling);
        m_stopgo_size.x = -1;
    }

    void poll(bool& _exit)
    {
#ifdef GLFW_GL3
        if (glfwWindowShouldClose(m_window))
            m_exit = true;
        glfwPollEvents();
#else
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                m_exit = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_window))
                m_exit = true;
        }
#endif

        if (m_exit)
        {
            if (Globals::ProgramStates::tcp_success)
            {
                uint8_t stopData[] = { 0,0,0,0,0,0,0,0 };
                Globals::CanFrame stopFrame(0x00000300, 5, stopData);
                addFrameToQueue(stopFrame);
            }
            _exit = true;
        }
    }

    void newFrame()
    {
        // Start the Dear ImGui frame
#ifdef GLFW_GL3
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
#else
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
#endif
        ImGui::NewFrame();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        //ImGui::PushFont(m_font);
    }

    void render()
    {
        // Rendering every loop

        ImGui::PopStyleVar();
        //ImGui::PopFont();
        ImGui::EndFrame();
        ImGui::Render();

#ifdef GLFW_GL3
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(m_window);
#else
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
        SDL_RenderClear(m_renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(m_renderer);
#endif
    }

    void cleanup()
    {
        // Cleanup
#ifdef GLFW_GL3
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(m_window);
        glfwTerminate();
#else
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
#endif
    }

    void draw(bool& _exit) {

        poll(_exit);
        newFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        m_main_size = { viewport->Size.x, viewport->Size.y - m_status_size.y };
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(m_main_size);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        bool main_window_success = (ImGui::Begin("MainWindow", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking));
        ImGui::PopStyleVar();

        if (main_window_success)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0, 0));

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Datei")) {
                    if (ImGui::MenuItem("Einstellungen")) m_draw_settings = true;
                    ImGui::Separator();
                    ImGui::MenuItem("Beenden", "Alt+F4", &m_exit);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Fenster")) {
                    addModuleMenuItems();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Hilfe")) {
                    if (ImGui::MenuItem("Über MäCAN Control Panel")) m_draw_info = true;
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
        }
        ImGui::End();

        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, m_status_size.y));
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - m_status_size.y));
        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
        if (ImGui::Begin("Status Bar", NULL, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            // m_status_size.x = ImGui::GetWindowSize().x;
            // m_status_size.x = viewport->Size.x - m_stopgo_size.x;

            if (!Globals::ProgramStates::tcp_started && !Globals::ProgramStates::tcp_success)
            {
                if (ImGui::SmallButton("Verbinden")) Globals::ProgramCmds::tcp_connect = true;
                ImGui::SameLine();

                if (Globals::ProgramStates::tcp_error_code == WSAECONNRESET)
                {
                    ImGui::Text("Verbindung zum Host verloren. TCP-Code: %d", Globals::ProgramStates::tcp_error_code);
                }
                else if (Globals::ProgramStates::tcp_error_code == WSAEWOULDBLOCK)
                {
                    ImGui::Text("Keine Verbindung zum Host möglich. TCP-Code: %d", Globals::ProgramStates::tcp_error_code);
                }
                else if (Globals::ProgramStates::tcp_error_code != 0)
                {
                    ImGui::Text("Fehler. TCP-Code: %d", Globals::ProgramStates::tcp_error_code);
                }
                else
                {
                    ImGui::Text("Nicht verbunden.");
                }
            }
            else if (Globals::ProgramStates::tcp_started && !Globals::ProgramStates::tcp_success)
            {
                if (ImGui::SmallButton("Trennen")) Globals::ProgramCmds::tcp_disconnect = true;
                ImGui::SameLine();
                ImGui::Text("Verbindung wird hergestellt. TCP-Code: %d", Globals::ProgramStates::tcp_error_code);
            }
            else if (Globals::ProgramStates::tcp_success)
            {
                if (ImGui::SmallButton("Trennen")) Globals::ProgramCmds::tcp_disconnect = true;
                ImGui::SameLine();
                ImGui::Text("Verbunden mit IP %s, Port %d.", Globals::ProgramStates::connected_ip.c_str(), Globals::ProgramStates::connected_port);
            }
        }
        ImGui::SameLine();

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - m_stopgo_size.x, ImGui::GetWindowPos().y));
        if (ImGui::BeginChild("StopGo Buttons", ImVec2(-1, m_status_size.y), true, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            if (Globals::ProgramStates::tcp_success)
            {
                ImGui::Text("Spannung: %.2f V, Strom: %.2f A", DeviceManager::voltage, DeviceManager::current);
                ImGui::SameLine();
            }
            bool b_stop = !Globals::ProgramStates::track_power && Globals::ProgramStates::tcp_success;
            bool b_go = Globals::ProgramStates::track_power && Globals::ProgramStates::tcp_success;

            if (b_stop)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            }
            if (ImGui::SmallButton("STOP") && Globals::ProgramStates::tcp_success) addFrameToQueue(m_stopFrame);
            if (b_stop) ImGui::PopStyleColor(2);
            ImGui::SameLine();

            if (b_go)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.60f, 0.10f, 1.00f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.00f, 0.60f, 0.10f, 1.00f));
            }
            if (ImGui::SmallButton("GO") && Globals::ProgramStates::tcp_success) addFrameToQueue(m_goFrame);
            if (b_go) ImGui::PopStyleColor(2);
            ImGui::SameLine();
            m_stopgo_size.x = ImGui::GetCursorPosX();
        }
        ImGui::EndChild();
        ImGui::End();
        if (m_draw_settings) drawSettings();
        if (m_draw_info) drawInfo();

        drawModules();

        render();
    }

    void drawSettings()
    {
        //Settings Window
        if (!ImGui::Begin("Einstellungen", &m_draw_settings, DialogWindowFlag)) {

            ImGui::End();
            return;
        }
        static bool on_open = true;
        static Globals::ProgramSettings local_settings;

        if (on_open) {
            on_open = false;
            local_settings.getSettings();
        }

        ImGui::PushFont(m_bold_font);
        ImGui::Text("Netzwerkeinstellungen");
        ImGui::PopFont();

        char _IP[16];
        memcpy(_IP, local_settings.tmp_tcp_ip.c_str(), 16);
        ImGui::InputText("IP", _IP, IM_ARRAYSIZE(_IP));
        local_settings.tmp_tcp_ip = _IP;

        ImGui::InputInt("Port", &(local_settings.tmp_tcp_port));

        ImGui::Separator();

        ImGui::PushFont(m_bold_font);
        ImGui::Text("Trace-Einstellungen");
        ImGui::PopFont();
        ImGui::Checkbox("Trace anzeigen", &(local_settings.tmp_trace));
        if (ImGui::Combo("Trace-Level", &(local_settings.tmp_trace_level), "Error\0Warnung\0Info\0")) {

        }

        ImGui::Separator();

        if (ImGui::Button("OK")) {
            local_settings.applySettings();
            Globals::ProgramSettings::has_changed = true;
            m_draw_settings = false;
            on_open = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Abbrechen")) {
            m_draw_settings = false;
            on_open = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Übernehmen")) {
            local_settings.applySettings();
            Globals::ProgramSettings::has_changed = true;
        }
        ImGui::End();
    }

    void drawInfo()
    {
        if (ImGui::Begin("Über MäCAN Control Panel", &m_draw_info, DialogWindowFlag))
        {
            ImGui::PushFont(m_bold_font);
            ImGui::Text("MäCAN Control Panel");
            ImGui::PopFont();
            ImGui::Text("Version: %s", VERSION);
            ImGui::Text("Commit: %s", COMMIT_CODE);
            ImGui::Text("© 2022 Maximilian Goldschmidt");
            if (ImGui::SmallButton("https://github.com/ixam97"))
                ShellExecute(0, 0, L"https://github.com/ixam97", 0, 0, SW_SHOW);
            ImGui::SameLine();
            if (ImGui::SmallButton("ixam97@ixam97.de"))
                ShellExecute(0, 0, L"mailto://ixam97@ixam97.de", 0, 0, SW_SHOW);
        }
        ImGui::End();

    }

    void addFrameToQueue(Globals::CanFrame _frame)
    {
        if (Globals::ProgramStates::tcp_success)
            m_frameOutQueue.push(_frame);
    }

    bool getFrame(Globals::CanFrame& _frame)
    {
        if (m_frameOutQueue.size() == 0) return false;
        else {
            _frame = m_frameOutQueue.front();
            m_frameOutQueue.pop();
            return true;
        }
    }

    void customSytle(ImGuiStyle* dst)
    {
        ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.0f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.95f);
        colors[ImGuiCol_Tab] = colors[ImGuiCol_TitleBgActive];
        colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_TabActive] = colors[ImGuiCol_MenuBarBg];
        colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_TitleBg];
        colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_MenuBarBg];
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }
}

