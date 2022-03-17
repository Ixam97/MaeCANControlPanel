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
 * Commit: [2022-03-17.1]
 */

#ifndef GUI_H_
#define GUI_H_

#include "globals.h"

namespace GUI
{

    void setup(int x_res, int y_res, const char* window_name);

    // Draw the main window an ints sub windows
    void draw(bool& _exit);

    // Cleanup at program end
    void cleanup();

    // Get a CAN frame from the internal output buffer
    bool getFrame(Globals::CanFrame& _frame);
}

#endif
