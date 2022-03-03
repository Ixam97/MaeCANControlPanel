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
 * canframe.cpp
 * (c)2022 Maximilian Goldschmidt
 */

#include "canframe.h"
#include <cstring>

canFrame::canFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash)
{
    cmd = _cmd;
    resp = _resp;
    dlc = _dlc;
    can_hash = _hash;
    memcpy(data, _data, 8);
    id = (cmd << 17) | ((_resp & 1) << 16) | _hash;
}

canFrame::canFrame(uint32_t _id, uint8_t _dlc, uint8_t _data[8])
{
    id = _id;
    dlc = _dlc;
    memcpy(data, _data, 8);
    cmd = (uint8_t)(id >> 17);
    resp = (uint8_t)(id >> 16) & 0b1;
    can_hash = (uint16_t)id & 0xffff;
}

canFrame::canFrame()
{
}

canFrame newCanFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash)
{
    canFrame outFrame(_cmd, _resp, _dlc, _data, _hash);
    return outFrame;
}

canFrame newCanFrame(uint8_t _cmd, uint8_t _resp, uint16_t _hash)
{
    uint8_t out_data[] = { 0,0,0,0,0,0,0,0 };
    canFrame outFrame(_cmd, _resp, 0, out_data);
    return outFrame;
}