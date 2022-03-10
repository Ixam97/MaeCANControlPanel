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
 * Commit: [2022-03-10.1]
 */

#pragma once
#include <string>
#include "../version.h"

 /*
 * Address areas of local IDs
 */
#define MM_ACC 			0x3000	/**< Address range for M‰rklin Motorola accessorys */
#define DCC_ACC 		0x3800	/**< Address range for DCC accessorys */
#define MM_TRACK 		0x0000	/**< Address range for M‰rklin Motorola locos */
#define DCC_TRACK 		0xC000	/**< Address range for DCC locos */

 /*
 * M‰rklin CAN commands
 */
#define SYS_CMD			0x00 	/**< System commands */
#define SYS_STOP 	0x00 	/**< System subcommand "STOP" */
#define SYS_GO		0x01	/**< System subcommand "GO" */
#define SYS_HALT	0x02	/**< System subcommand "Emergency stop" */
#define SYS_STAT	0x0b	/**< System subcommand to send changed config values or meassurement values */
#define CMD_SWITCH_ACC 	0x0b	/**< Command to switch accessorys */
#define CMD_S88_EVENT	0x11	/**< Command to send S88 events */
#define CMD_PING 		0x18	/**< Command to send or request pings */
#define CMD_CONFIG		0x1d	/**< Command to send or request config or meassurement deffinitions */
#define CMD_BOOTLOADER	0x1B	/**< M‰rklin bootloader command */
#define CMD_MCAN_BOOT	0x40	/**< MCAN bootloader command */

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

namespace Interface
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
