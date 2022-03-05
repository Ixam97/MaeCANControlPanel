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
 * configworker.h
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-05.1]
 */

#include "canframe.h"
#include <vector>
#include <queue>

#pragma once
class ConfigWorker
{
private:
    static inline std::vector<canFrame> m_frameInVector;
    static inline std::queue<canFrame> m_frameOutQueue;
    static inline uint8_t m_current_index = 0;
    static inline bool m_busy = false;

    static void addFrameToQueue(canFrame _frame);
public:
    static inline uint32_t uid = 0;

    static bool busy() { return m_busy; }

    // reset the config worker
    static void reset();

    // add CAN frame to internal input buffer and process it if ready
    static void addFrame(canFrame _frame);

    // get CAN frame from internal output buffer
    static bool getFrame(canFrame& _frame);

    // Define a device to get config from
    static void workOn(uint32_t _uid);
};

