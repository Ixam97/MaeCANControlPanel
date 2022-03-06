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
 * globals.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-06.1]
 */

#include "globals.h"

ProgramSettings global_settings;
ProgramStates global_states;
ProgramCmds global_cmds;
UpdaterInterface update_interfrace;

std::vector<canDevice> device_list;
std::vector<readingsRequestInfo> readings_request_list;

void logInfo(const char* _trace, long _return_code)
{
    if (global_settings.trace && global_settings.trace_level >= 2)
    {
        printf("[Info] %s", _trace);
        if (_return_code) printf(" Error code: %d", _return_code);
        printf("\n");
    }

}
void logWarn(const char* _trace, long _return_code)
{
    if (global_settings.trace && global_settings.trace_level >= 1)
    {
        printf("[Warning] %s", _trace);
        if (_return_code) printf(" Error code: %d", _return_code);
        printf("\n");
    }
}
void logError(const char* _trace, long _return_code)
{
    if (global_settings.trace && global_settings.trace_level >= 0)
    {
        printf("[Error] %s", _trace);
        if (_return_code) printf(" Error code: %d", _return_code);
        printf("\n");
    }

}

uint32_t getUidFromData(uint8_t _data[8])
{
    return (_data[0] << 24) | (_data[1] << 16) | (_data[2] << 8) | _data[3];
}
