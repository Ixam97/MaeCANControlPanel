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
 * Commit: [2022-03-03.1]
 */

#include "gui.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

GUI::GUI(int x_res, int y_res, const char* window_name) {

    SetProcessDPIAware();
    m_scaling = (float)GetDeviceCaps(GetDC(NULL), LOGPIXELSX) / 96;


    m_x_res = x_res;
    m_y_res = y_res;

    // m_can = _can;

    // Init everything for the UI

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        //return false;
    }

    // Setup window

    m_window = SDL_CreateWindow(window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_x_res, m_y_res, window_flags);

    // Setup SDL_Renderer instance
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (m_renderer == NULL)
    {
        SDL_Log("Error creating SDL_Renderer!");
        //return false;
    }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().IniFilename = "MaeCANControlPanelGUI.ini";

    

    //ImGuiBackendFlags_PlatformHasViewports

    // Setup Dear ImGui style
    customSytle();


    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(m_window, m_renderer);
    ImGui_ImplSDLRenderer_Init(m_renderer);

    m_font = ImGui::GetIO().Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\seguisym.ttf", floor(18.0f * m_scaling));
    m_consolas = ImGui::GetIO().Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\consola.ttf", floor(14.0f * m_scaling));
}

void GUI::poll(bool* _exit)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            m_exit = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_window))
            m_exit = true;
    }

    if (m_exit)
    {
        if (global_states.tcp_success)
        {
            uint8_t stopData[] = { 0,0,0,0,0,0,0,0 };
            canFrame stopFrame(0x00000300, 5, stopData);
            addFrameToQueue(stopFrame);
        }
        *_exit = true;
    }   
}

void GUI::newFrame()
{
    // Start the Dear ImGui frame

    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushFont(m_font);
}

void GUI::render()
{
    // Rendering every loop

    ImGui::PopStyleVar();
    ImGui::PopFont();

    ImGui::Render();

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(m_renderer);
}

void GUI::cleanup()
{
    // Cleanup
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void GUI::drawMainWindow() {

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    m_main_size = { viewport->Size.x, viewport->Size.y - m_status_size.y};
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(m_main_size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool main_window_success = (ImGui::Begin("MainWindow", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus ));
    ImGui::PopStyleVar();

    if (main_window_success)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0,0));

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Datei")) {
                if (ImGui::MenuItem("Einstellungen")) m_draw_settings = true;
                ImGui::Separator();
                ImGui::MenuItem("Beenden", "Alt+F4", &m_exit);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Fenster")) {
                ImGui::MenuItem("CAN-Monitor","", &m_draw_consoles);
                ImGui::MenuItem("Gerätemanager", "", &m_draw_device_manager);
#ifdef _DEBUG
                ImGui::MenuItem("Debug","", &m_draw_debug_monitor);
#endif
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Hilfe")) {
                if (ImGui::MenuItem("Über MäCAN Control Panel")) m_draw_info = true;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();

            //ImGui::Text("IP: %s", global_settings.tcp_ip.c_str());
            //ImGui::Text("Port: %d", global_settings.tcp_port);

        }
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x - m_stopgo_size.x, -1));
    ImGui::SetNextWindowPos(ImVec2(0, viewport->Size.y - m_status_size.y));

    if (ImGui::Begin("Status Bar", NULL, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        m_status_size = ImGui::GetWindowSize();

        if (!global_states.tcp_started && !global_states.tcp_success)
        {
            if (ImGui::SmallButton("Verbinden")) global_cmds.tcp_connect = true;
            ImGui::SameLine();
            
            if (global_states.tcp_error_code == WSAECONNRESET)
            {
                ImGui::Text("Verbindung zum Host verloren. TCP-Code: %d", global_states.tcp_error_code);
            }
            else if (global_states.tcp_error_code == WSAEWOULDBLOCK)
            {
                ImGui::Text("Keine Verbindung zum Host möglich. TCP-Code: %d", global_states.tcp_error_code);
            }
            else if (global_states.tcp_error_code != 0)
            {
                ImGui::Text("Fehler. TCP-Code: %d", global_states.tcp_error_code);
            }
            else
            {
                ImGui::Text("Nicht verbunden.");
            }
        }
        else if (global_states.tcp_started && !global_states.tcp_success)
        {
            if (ImGui::SmallButton("Trennen")) global_cmds.tcp_disconnect = true;
            ImGui::SameLine();
            ImGui::Text("Verbindung wird hergestellt. TCP-Code: %d", global_states.tcp_error_code);
        }
        else if (global_states.tcp_success)
        {
            if (ImGui::SmallButton("Trennen")) global_cmds.tcp_disconnect = true;
            ImGui::SameLine();
            ImGui::Text("Verbunden mit IP %s, Port %d.", global_states.connected_ip.c_str(), global_states.connected_port);
        }
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(-1, m_status_size.y));
    ImGui::SetNextWindowPos(ImVec2(m_status_size.x, viewport->Size.y - m_status_size.y));

    if (ImGui::Begin("StopGo Buttons", NULL, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        m_stopgo_size = ImGui::GetWindowSize();

        if (global_states.tcp_success)
        {
            for (canDevice n : device_list)
            {
                if ((n.type == 0x0010 || n.type == 0x0011) && n.vec_readings_channels.size() == 3)
                {
                    ImGui::Text("Spannung: %.2f V, Strom: %.2f A", (float)(n.vec_readings_channels[1].current_value * n.vec_readings_channels[1].value_factor) + std::stof(n.vec_readings_channels[1].s_min), (float)(n.vec_readings_channels[0].current_value * n.vec_readings_channels[0].value_factor) + std::stof(n.vec_readings_channels[0].s_min));
                    ImGui::SameLine();
                }
            }
        }
        
        if (!global_states.track_power && global_states.tcp_success) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        if (ImGui::SmallButton("STOP") && global_states.tcp_success) addFrameToQueue(m_stopFrame);
        if (!global_states.track_power && global_states.tcp_success) ImGui::PopStyleColor();
        ImGui::SameLine();

        if (global_states.track_power && global_states.tcp_success) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.60f, 0.10f, 1.00f));
        if (ImGui::SmallButton("GO") && global_states.tcp_success) addFrameToQueue(m_goFrame);
        if (global_states.track_power && global_states.tcp_success) ImGui::PopStyleColor();
   
    }
    ImGui::End();



    if (m_draw_settings) drawSettings();
    if (m_draw_consoles) drawConsoles();
    if (m_draw_device_manager) drawDeviceManager(); 
    if (m_draw_info) drawInfo();
#ifdef _DEBUG
    // if (m_draw_debug_monitor) ImGui::ShowMetricsWindow(&m_draw_debug_monitor);
    if (m_draw_debug_monitor) drawDebugMonitor();
#endif
}

void GUI::drawSettings()
{
    //Settings Window
    if (!ImGui::Begin("Einstellungen", &m_draw_settings, DialogWindowFlag)) {

        ImGui::End();
        return;
    }
    static bool on_open = true;
    static ProgramSettings local_settings;

    if (on_open) {
        on_open = false;
        local_settings = global_settings;
    }

    ImGui::Text("Netzwerkeinstellungen");
    
    char _IP[16];
    memcpy(_IP, local_settings.tcp_ip.c_str(), 16);
    ImGui::InputText("IP", _IP, IM_ARRAYSIZE(_IP));
    local_settings.tcp_ip = _IP;

    ImGui::InputInt("Port", &(local_settings.tcp_port));

    ImGui::Separator();

    

    ImGui::Text("Trace-Einstellungen");
    ImGui::Checkbox("Trace anzeigen", &(local_settings.trace));
    if (ImGui::Combo("Trace-Level", &(local_settings.trace_level), "Error\0Warnung\0Info\0")) {
        
    }

    ImGui::Separator();

    if (ImGui::Button("OK")) {
        global_settings = local_settings;
        global_settings.has_changed = true;
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
        global_settings = local_settings;
        global_settings.has_changed = true;
    }
    ImGui::End();
    
}

void GUI::drawConsoles()
{
    if (!ImGui::Begin("CAN-Monitor", &m_draw_consoles, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }
    static bool AutoScroll = true;
    static bool EasyMode = false;
    static int max_console_lines = 200;

    static bool JumpToBottom = false;
    static size_t vector_size = 0;
    bool copy_to_clipboard = false;
    static int cmd_filter = 0;
    static bool b_cmd_filter = false;

    while (m_consoleVector.size() > max_console_lines)
    {
        for (size_t i = 0; i < m_consoleVector.size() - 1; i++)
        {
            m_consoleVector[i] = m_consoleVector[i + 1];
        }
        m_consoleVector.pop_back();
    }
    
    if (vector_size != m_consoleVector.size()) {
        vector_size = m_consoleVector.size();
        if (AutoScroll) JumpToBottom = true;
    }
    
    if (ImGui::BeginPopup("Optionen")) {
        ImGui::Checkbox("Automatisches Scrollen", &AutoScroll);
        ImGui::Checkbox("Vereinfachter Modus", &EasyMode);
        ImGui::InputInt("Max. Zeilen", &max_console_lines, 1, 10, ImGuiInputTextFlags_EnterReturnsTrue);
        if (max_console_lines < 0) max_console_lines = 0;
        
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Filter")) {
        ImGui::Checkbox("Befehlsfilter", &b_cmd_filter);
        ImGui::InputInt("##", &cmd_filter, 1, 1, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::EndPopup();
    }

    if (ImGui::SmallButton("Optionen")) ImGui::OpenPopup("Optionen");
    ImGui::SameLine();
    if (ImGui::SmallButton("Filter")) ImGui::OpenPopup("Filter");
    ImGui::SameLine();
    if (ImGui::SmallButton("Leeren")) m_consoleVector.resize(0);
    ImGui::SameLine();
    copy_to_clipboard = ImGui::SmallButton("In Zwischenablage kopieren");

    ImGui::Separator();

    //const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("Scrolling Region");
    ImGui::PushFont(m_consolas);

    if (copy_to_clipboard) ImGui::LogToClipboard();

    for (canFrame n : m_consoleVector)
    {
        if (b_cmd_filter)
        {
            if (n.cmd == cmd_filter)
                consoleDecoder(n, EasyMode);
        }
        else 
            consoleDecoder(n, EasyMode);
    }

    if (copy_to_clipboard) ImGui::LogFinish();

    if (JumpToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
        ImGui::SetScrollHereY(1.0f);
    JumpToBottom = false;

    ImGui::PopFont();
    ImGui::EndChild();

    ImGui::End();
}

void GUI::drawDeviceManager()
{

    static int current_index = 0;
    if (!ImGui::Begin("Gerätemanager", &m_draw_device_manager, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Ping")) addFrameToQueue(newCanFrame(CMD_PING, 0));
    ImGui::SameLine();
    if (ImGui::Button("Liste zurücksetzen"))
    {
        current_index = 0;
        global_cmds.config_worker_reset = true;
    }

    ImGui::Separator();

    if (ImGui::BeginListBox("##", ImVec2(floor(200 * m_scaling), -1)))
    {
        for (int i = 0; i < device_list.size(); i++)
        {
            if (device_list[i].data_complete)
            {
                const bool is_selected = (current_index == i);
                std::stringstream ss_list_label;
                //ss_list_label << "0x" << std::hex << std::uppercase << n.uid << " Version: " << std::dec << (int)n.version_h << "." << (int)n.version_l;
                ss_list_label << device_list[i].name << " #" << std::dec << device_list[i].serialnbr;
                if (ImGui::Selectable(ss_list_label.str().c_str(), is_selected))
                {
                    current_index = i;
                }
            }
        }
        ImGui::EndListBox();
    }
    ImGui::SameLine();
    ImGui::BeginChild("DeviceInfoRegion");

    if (device_list.size() > 0 && device_list[current_index].data_complete)
    {
        ImGui::Text(device_list[current_index].name.c_str());
        ImGui::Separator();
        ImGui::Text("Artikelnummer: %s ", device_list[current_index].item.c_str());
        ImGui::Text("Seriennummer: %d", device_list[current_index].serialnbr);
        ImGui::Text("Version: %d.%d", device_list[current_index].version_h, device_list[current_index].version_l);
        ImGui::Text("UID: 0x%08X", device_list[current_index].uid);
        ImGui::Text("Konfigurationskanäle: %d, Messwertkanäle: %d", device_list[current_index].num_config_channels, device_list[current_index].num_readings_channels);
    
        if (device_list[current_index].num_readings_channels > 0)
        {
            ImGui::Separator();
            for (readingsChannel n : device_list[current_index].vec_readings_channels)
            {
                float true_value = (n.current_value * n.value_factor) + std::stof(n.s_min);
                float relative_value = (true_value - std::stof(n.s_min)) / (std::stof(n.s_max) - std::stof(n.s_min));
                ImGui::Text("%s: %.2f %s", n.label.c_str(), true_value, n.unit.c_str());

                if (n.current_value > n.points[3]) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, byteToColor(n.colors[3]));
                else if (n.current_value > n.points[2]) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, byteToColor(n.colors[2]));
                else if (n.current_value > n.points[1]) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, byteToColor(n.colors[1]));
                else ImGui::PushStyleColor(ImGuiCol_PlotHistogram, byteToColor(n.colors[0]));
                ImGui::ProgressBar(relative_value, ImVec2(-1, 0), "");
                ImGui::PopStyleColor();                
            }
        }
        if (device_list[current_index].num_config_channels > 0)
        {
            ImGui::Separator();
            for (int i = 0; i < device_list[current_index].vec_config_channels.size(); i++)
            {
                //if (n.unit == "") ImGui::Text("%s", n.label.c_str());
                //else ImGui::Text("%s (%s)", n.label.c_str(), n.unit.c_str());

                // Drop down
                if (device_list[current_index].vec_config_channels[i].type == 1)
                {
                    if (ImGui::Combo(device_list[current_index].vec_config_channels[i].label.c_str(), &(device_list[current_index].vec_config_channels[i].wanted_value), device_list[current_index].vec_config_channels[i].dropdown_options_separated_by_zero.c_str()))
                        device_list[current_index].vec_config_channels[i].request_sent = false;
                }
                // Int slider
                else if (device_list[current_index].vec_config_channels[i].type == 2)
                {
                    if (ImGui::SliderInt(device_list[current_index].vec_config_channels[i].label.c_str(), &(device_list[current_index].vec_config_channels[i].wanted_value), device_list[current_index].vec_config_channels[i].min, device_list[current_index].vec_config_channels[i].max))
                        device_list[current_index].vec_config_channels[i].request_sent = false;
                    ImGui::SameLine(); helpMarker("STRG + Klick, um Zahlenwerte einzugeben.");
                }
            }
        }
    
    }

    ImGui::EndChild();

    ImGui::End();
}

void GUI::drawDebugMonitor() {
    if (!ImGui::Begin("DebugMonitor", &m_draw_debug_monitor))
    {
        ImGui::End();
        return;
    }
    ImGui::Text("Global states:");
    ImGui::Text("TCP error code: %d", global_states.tcp_error_code);
    ImGui::Text("TCP started: %d", global_states.tcp_started);
    ImGui::Text("TCP success: %d", global_states.tcp_success);
    ImGui::Text("Track Power: %d", global_states.track_power);
    ImGui::Separator();
    ImGui::Text("Lists:");
    ImGui::Text("Device list size: %d", device_list.size());
    ImGui::Text("Readings request list size: %d", readings_request_list.size());
    ImGui::Text("CAN monitor lines: %d", m_consoleVector.size());

    ImGui::End();
}

void GUI::drawInfo()
{
    if (ImGui::Begin("Über MäCAN Control Panel", &m_draw_info, DialogWindowFlag)) {
        ImGui::Text("MäCAN Control Panel");
        ImGui::Text("Version: %s", VERSION);
        ImGui::Text("Commit: %s", COMMIT_CODE);
        ImGui::Text("© 2022 Maximilian Goldschmidt");
        ImGui::Text("https://github.com/ixam97, ixam97@ixam97.de");
    }
    ImGui::End();
    
}

void GUI::addFrameToQueue(canFrame _frame)
{
    if (global_states.tcp_success)
        m_frameOutQueue.push(_frame);
}

bool GUI::getFrame(canFrame& _frame)
{
    if (m_frameOutQueue.size() == 0) return false;
    else {
        _frame = m_frameOutQueue.front();
        m_frameOutQueue.pop();
        return true;
    }
}

ImVec4 GUI::byteToColor(uint8_t _byte)
{
    return ImVec4((float)((_byte & 0xc0) >> 6) / 0b11, (float)((_byte & 0x30) >> 4) / 0b11, (float)((_byte & 0x0c) >> 2) / 0b11, 1.00);
}

void GUI::customSytle(ImGuiStyle* dst)
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

void GUI::addFrameToConsoleVector(canFrame _frame)
{
    if (m_draw_consoles)
    {
        m_consoleVector.push_back(_frame);
    }
}

void GUI::helpMarker(const char* desc)
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


