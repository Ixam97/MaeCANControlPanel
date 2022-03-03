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
 * Commit: [2022-03-03.1]
 */

#include "canframe.h"
#include <vector>
#include <queue>

#pragma once
class ConfigWorker
{
private:
    std::vector<canFrame> m_frameInVector;
    std::queue<canFrame> m_frameOutQueue;
    uint8_t m_current_index = 0;
    void addFrameToQueue(canFrame _frame);
public:
    uint32_t uid = 0;
    bool busy = false;

    // reset the config worker
    void reset();

    // add CAN frame to internal input buffer and process it if ready
    void addFrame(canFrame _frame);

    // get CAN frame from internal output buffer
    bool getFrame(canFrame& _frame);

    // Define a device to get config from
    void workOn(uint32_t _uid);
};

