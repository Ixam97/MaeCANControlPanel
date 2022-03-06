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
 * globals.h
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-06.1]
 */

#pragma once

#include <string>
#include <vector>
#include "../version.h"

#define VERSION stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD)

#define T_SLIDER 1
#define T_DROPDOWN 2

#define PARSE_ERR_FILE_OPEN -1
#define PARSE_ERR_CHECKSUM -2
#define PARSE_ERR_CONVERSION -3
#define PARSE_ERR_NOT_SUPPORTED -4

#define MCAN_UPDATE_IDLE 0
#define MCAN_UPDATE_IN_PROGRESS 1
#define MCAN_UPDATE_FAILURE_ERROR 2
#define MCAN_UPDATE_FAILURE_INCOMPATIBLE 3
#define MCAN_UPDATE_SUCCESS 4
#define MCAN_UPDATE_INIT 5

struct ProgramSettings 
{
	int tcp_port = 15731;
	std::string tcp_ip = "192.168.0.5";
	bool trace = false;
	int trace_level = 0;

	bool has_changed = false;			// Set to true if settings have been modified and need to be saved.
	unsigned int request_interval = 5;	// Default interval of 5 sec.
};

struct ProgramStates 
{
	bool tcp_success = false;
	bool tcp_started = false;
	int tcp_error_code = 0;
	bool track_power = false;
	std::string connected_ip = "";
	int connected_port = 0;
	bool new_request_list_entry = false;
};

struct ProgramCmds 
{
	bool tcp_connect = false;
	bool tcp_disconnect = false;
	bool config_worker_reset = false;
};

struct configChannel
{
	uint8_t channel_index;
	uint16_t type;
	int current_value;
	int wanted_value;
	uint8_t num_options;
	std::vector<std::string> dropdown_options;
	std::string dropdown_options_separated_by_zero;
	std::string label;
	std::string unit;
	uint16_t min, max;
	std::string s_min, s_max;
	bool request_sent = true;
};

struct readingsChannel
{
	uint8_t channel_index;
	int16_t current_value;
	float value_factor;
	std::string label;
	std::string unit;
	int8_t power;
	uint8_t colors[4];
	int16_t points[5];
	std::string s_min, s_max;
};

struct canDevice
{
	std::string name;
	std::string item;
	uint16_t type;
	uint32_t serialnbr;
	uint8_t version_h;
	uint8_t version_l;
	uint32_t uid;
	int num_config_channels = -1;
	int num_readings_channels;
	std::vector<configChannel> vec_config_channels;
	std::vector<readingsChannel> vec_readings_channels;
	bool data_complete = false;
	bool selected = false;
};

struct readingsRequestInfo
{
	uint32_t uid;
	uint8_t channel;
};

struct UpdaterInterface
{
	uint32_t uid;
	uint16_t type;
	int status;
	bool do_update = false;
	bool do_abort = false;
	bool get_file_names = false;
	bool in_progress = false;
	float progress = 0.0f;
	std::vector<std::string> file_names;
	std::string file_name;

};

// Data to be accessable globaly
extern struct ProgramSettings global_settings;
extern struct ProgramStates global_states;
extern struct ProgramCmds global_cmds;
extern struct UpdaterInterface update_interfrace;

extern std::vector<canDevice> device_list;
extern std::vector<readingsRequestInfo> readings_request_list;

// Logging functions

// Log at info level
void logInfo(const char* _trace, long _return_code = 0);
// Log at warning level
void logWarn(const char* _trace, long _return_code = 0);
// Log at error level
void logError(const char* _trace, long _return_code = 0);

// Helper functions

// Convert can frame data (uint8_t[8]) into a 32bit UID
uint32_t getUidFromData(uint8_t _data[8]);
