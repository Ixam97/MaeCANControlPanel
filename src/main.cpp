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
 * main.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-10.1]
 */

#include "interface.h"
#include "can.h"
#include "gui.h"
#include "devicemanager.h"
#include "feedbackmonitor.h"
#include "busmonitor.h"

#include <thread>
#include <chrono>

#define CAN_FRAME_TIMEOUT 10 // Timeout in ms

HWND consoleWindow;

bool b_exit = false;

void logicLoop()
{

    while (!b_exit || (CAN::getQueueLength(OUTQUEUE) > 0))
    {
        std::chrono::steady_clock::time_point now_time = std::chrono::high_resolution_clock::now();
        static std::chrono::steady_clock::time_point last_tcp_check_time = now_time;
        std::chrono::duration<double> tcp_check_duration = now_time - last_tcp_check_time;

        if (tcp_check_duration.count() >= 5 && Interface::ProgramStates::tcp_success)
        {
            last_tcp_check_time = now_time;
            CAN::TCPCheckConnection();
        }

        // Check connection establishment
        if (Interface::ProgramStates::tcp_started) CAN::TCPCheckConnection();

        // Start connection establishment
        if (Interface::ProgramCmds::tcp_connect) 
        {
            Interface::ProgramCmds::tcp_connect = false;
            Interface::ProgramStates::tcp_started = CAN::TCPConnect();
        }

        // Disconnect
        if (Interface::ProgramCmds::tcp_disconnect) 
        {
            Interface::ProgramCmds::tcp_disconnect = false;
            CAN::TCPDisconnect();
        }        

        // Save settings after they changed
        if (Interface::ProgramSettings::has_changed)
        {
            if (Interface::ProgramSettings::trace) ShowWindow(consoleWindow, SW_SHOW);
#ifndef _DEBUG
            else ShowWindow(consoleWindow, SW_HIDE);
#endif
            Interface::writeIni();
        }

        // Put new frames into queue
        CAN::TCPReadFrame();

        // Get new incoming frame from queue and pass to modules
        if (CAN::getQueueLength(INQUEUE))
        {
            Interface::CanFrame new_in_frame = CAN::processQueue(INQUEUE);

            BusMonitor::addFrame(new_in_frame);
            DeviceManager::addFrame(new_in_frame);
            FeedbackMonitor::addFrame(new_in_frame);

            if (new_in_frame.cmd == SYS_CMD &&new_in_frame.resp == 1 && new_in_frame.dlc == 5)
            {
                if (new_in_frame.data[4] == SYS_GO) Interface::ProgramStates::track_power = true;
                else if (new_in_frame.data[4] == SYS_STOP) Interface::ProgramStates::track_power = false;
            }
        }

        // looping functions of modules
        DeviceManager::loop();

        // get outbound frames from modules
        Interface::CanFrame tmp_frame;
        while (GUI::getFrame(tmp_frame))
            CAN::addFrameToQueue(tmp_frame, OUTQUEUE);
        while (FeedbackMonitor::getFrame(tmp_frame))
            CAN::addFrameToQueue(tmp_frame, OUTQUEUE);
        while (BusMonitor::getFrame(tmp_frame))
            CAN::addFrameToQueue(tmp_frame, OUTQUEUE);
        while (DeviceManager::getFrame(tmp_frame))
            CAN::addFrameToQueue(tmp_frame, OUTQUEUE);

        // Work on Queue to send
        if (CAN::getQueueLength(OUTQUEUE))
        {
            // Send queued frames after timeout
            static auto _last_time_sent = std::chrono::high_resolution_clock::now();
            auto _current_time = std::chrono::high_resolution_clock::now();

            if (_current_time - _last_time_sent > std::chrono::milliseconds(CAN_FRAME_TIMEOUT))
            {
                _last_time_sent = _current_time;
                Interface::CanFrame newOutFrame = CAN::processQueue(OUTQUEUE);
                if (newOutFrame.can_hash > 0) BusMonitor::addFrame(newOutFrame);
            }
        }
        Sleep(1);
    }
}

// Main code
int main(int argc, char** argv)
{
    Interface::loadIni();

    consoleWindow = GetConsoleWindow();
#ifndef _DEBUG
    if (!Interface::ProgramSettings::trace) ShowWindow(consoleWindow, SW_HIDE);
#endif
    GUI::setup(1900, 900, u8"M‰CAN Control Panel");

    // Put logic in seperate thread to not block it when resizing or moving the GUI window
    std::thread logicThread(logicLoop);

    // Default open windows
    DeviceManager::b_draw = true;
    BusMonitor::b_draw = true;

    // GUI loop
    while (!b_exit) {
        GUI::draw(b_exit);
    }

    logicThread.join();

    GUI::cleanup();
    CAN::TCPDisconnect();

    return 0;
}

