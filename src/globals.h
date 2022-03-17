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
 * interface.h
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-17.1]
 */

#pragma once
#include <string>
#include "../version.h"

//Address areas of local IDs
#define MM_ACC 			0x3000
#define DCC_ACC 		0x3800
#define MM_TRACK 		0x0000
#define DCC_TRACK 		0xC000

//M‰rklin CAN commands (https://www.maerklin.de/fileadmin/media/service/software-updates/cs2CAN-Protokoll-2_0.pdf)
#define CMD_SYS				0x00
#define CMD_LOCO_DIS		0x01
#define CMD_MFX_BIND		0x02
#define CMD_MFX_VERIFY		0x03
#define CMD_LOCO_SPEED		0x04
#define CMD_LOCO_DIR		0x05
#define CMD_LOCO_FN			0x06
#define CMD_READ_CONF		0x07
#define CMD_WRITE_CONF		0x08
#define CMD_SWITCH_ACC 		0x0B
#define CMD_CONFIG_ACC		0x0C
#define CMD_S88_POLL		0x10
#define CMD_S88_EVENT		0x11
#define CMD_SX1_EVENT		0x12
#define CMD_PING 			0x18
#define CMD_UPDATE_OFFER	0x19
#define CMD_R_CONFIG_DATA	0x1A
#define CMD_BOOTLOADER		0x1B
#define CMD_BL_TRACK		0x1C
#define CMD_CONFIG			0x1D
#define CMD_DATA_QUERY		0x20
#define CMD_DATA_STREAM		0x21
#define CMD_CONNECT_STREAM	0x22

//M‰rklin system sub commands
#define SYS_STOP 			0x00
#define SYS_GO				0x01
#define SYS_HALT			0x02
#define SYS_LOCO_HALT		0x03
#define SYS_LOCO_CYCLE_END	0x04
#define SYS_LOCO_DATAPROT	0x05
#define SYS_ACC_TIME		0x06
#define SYS_MFX_FAST_READ	0x07
#define SYS_TRACKPROT		0x08
#define SYS_MFX_REG_RESET	0x09
#define SYS_OVERLOAD		0x0A
#define SYS_STAT			0x0B
#define SYS_ID				0x0C
#define SYS_TIME			0x20
#define SYS_MFX_SEEK		0x30
#define SYS_SYSTEM_RESET	0x80

//M‰CAN CAN commands
#define CMD_MCAN_BOOT		0x40

// Version from version.h
#define VERSION stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD)

// M‰CAN updater hefile parser errors
#define PARSE_ERR_FILE_OPEN -1
#define PARSE_ERR_CHECKSUM -2
#define PARSE_ERR_CONVERSION -3
#define PARSE_ERR_NOT_SUPPORTED -4

// M‰CAN updater return codes
#define MCAN_UPDATE_IDLE 0
#define MCAN_UPDATE_IN_PROGRESS 1
#define MCAN_UPDATE_FAILURE_ERROR 2
#define MCAN_UPDATE_FAILURE_INCOMPATIBLE 3
#define MCAN_UPDATE_SUCCESS 4
#define MCAN_UPDATE_INIT 5

namespace Globals
{
	struct ProgramSettings
	{
		static inline int tcp_port = 15731;
		static inline std::string tcp_ip = "192.168.0.5";
		static inline bool trace = false;
		static inline int trace_level = 0;
		static inline bool has_changed = false;			// Set to true if settings have been modified and need to be saved.
		static inline unsigned int request_interval = 5;

		int tmp_tcp_port = 15731;
		std::string tmp_tcp_ip = "192.168.0.5";
		bool tmp_trace = false;
		int tmp_trace_level = 0;
		unsigned int tmp_request_interval = 5;

		void getSettings();
		void applySettings();
	};

	struct ProgramStates
	{
		static inline bool tcp_success = false;
		static inline bool tcp_started = false;
		static inline int tcp_error_code = 0;
		static inline bool track_power = false;
		static inline std::string connected_ip = "";
		static inline int connected_port = 0;
		static inline bool new_request_list_entry = false;
		static inline float gui_scaling = 1;
	};

	struct ProgramCmds
	{
		static inline bool tcp_connect = false;
		static inline bool tcp_disconnect = false;
		static inline bool config_worker_reset = false;
	};

	struct CanFrame {
		CanFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash = 0x300);
		CanFrame(uint32_t _id, uint8_t _dlc, uint8_t _data[8]);
		CanFrame() {}
		uint32_t id = 0;
		uint8_t cmd = 0;
		uint8_t resp = 0;
		uint16_t can_hash = 0;
		uint8_t dlc = 0;
		uint8_t data[8] = { 0,0,0,0,0,0,0,0 };
	};


	void logInfo(const char* _trace, long _return_code = 0);

	void logWarn(const char* _trace, long _return_code = 0);

	void logError(const char* _trace, long _return_code = 0);

	uint32_t getUidFromData(uint8_t _data[8]);

	void loadIni();

	void writeIni();

	bool writeToIni(std::string& section, std::string& name, std::string& value);
}
