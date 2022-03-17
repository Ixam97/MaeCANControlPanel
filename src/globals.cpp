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
 * interface.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-17.1]
 */

#include "globals.h"
#include "ini.h"

#define GLOBAL_SETTING_LOAD ini.get("global-settings")
#define GLOBAL_SETTING_WRITE ini["global-settings"]

mINI::INIFile ini_file("MaeCANControlPanelSettings.ini");
mINI::INIStructure ini;

namespace Globals 
{
	CanFrame::CanFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash)
	{
		cmd = _cmd;
		resp = _resp;
		dlc = _dlc;
		can_hash = _hash;
		for (int i = 0; i < 8; i++)
			data[i] = _data[i];
		id = (cmd << 17) | ((_resp & 1) << 16) | _hash;
	}

	CanFrame::CanFrame(uint32_t _id, uint8_t _dlc, uint8_t _data[8])
	{
		id = _id;
		dlc = _dlc;
		for (int i = 0; i < 8; i++)
			data[i] = _data[i];
		cmd = (uint8_t)(id >> 17);
		resp = (uint8_t)(id >> 16) & 0b1;
		can_hash = (uint16_t)id & 0xffff;
	}

	void ProgramSettings::getSettings()
	{
		tmp_tcp_port = tcp_port;
		tmp_tcp_ip = tcp_ip;
		tmp_trace = trace;
		tmp_trace_level = trace_level;
		tmp_request_interval = request_interval;
	}

	void ProgramSettings::applySettings() 
	{
		tcp_port = tmp_tcp_port;
		tcp_ip = tmp_tcp_ip;
		trace = tmp_trace;
		trace_level = tmp_trace_level;
		request_interval = tmp_request_interval;

	}

	void logInfo(const char* _trace, long _return_code)
	{
		if (ProgramSettings::trace && ProgramSettings::trace_level >= 2)
		{
			printf("[Info] %s", _trace);
			if (_return_code) printf(" Error code: %d", _return_code);
			printf("\n");
		}

	}
	void logWarn(const char* _trace, long _return_code)
	{
		if (ProgramSettings::trace && ProgramSettings::trace_level >= 1)
		{
			printf("[Warning] %s", _trace);
			if (_return_code) printf(" Error code: %d", _return_code);
			printf("\n");
		}
	}
	void logError(const char* _trace, long _return_code)
	{
		if (ProgramSettings::trace && ProgramSettings::trace_level >= 0)
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

	void loadIni()
	{
		if (ini_file.read(ini))
		{
			// File exists, load it
			bool missing_setting = false;

			if (GLOBAL_SETTING_LOAD.get("tcp-ip") == "") missing_setting = true; else ProgramSettings::tcp_ip = GLOBAL_SETTING_LOAD.get("tcp-ip");
			if (GLOBAL_SETTING_LOAD.get("tcp-port") == "") missing_setting = true; else ProgramSettings::tcp_port = stoi(GLOBAL_SETTING_LOAD.get("tcp-port"));
			if (GLOBAL_SETTING_LOAD.get("trace") == "") missing_setting = true; else ProgramSettings::trace = (bool)stoi(GLOBAL_SETTING_LOAD.get("trace"));
			if (GLOBAL_SETTING_LOAD.get("trace-level") == "") missing_setting = true; else ProgramSettings::trace_level = stoi(GLOBAL_SETTING_LOAD.get("trace-level"));

			if (missing_setting) writeIni();
		}
		else
		{
			// File does not exist, generate it.
			GLOBAL_SETTING_WRITE["tcp-ip"] = ProgramSettings::tcp_ip;
			GLOBAL_SETTING_WRITE["tcp-port"] = std::to_string(ProgramSettings::tcp_port);
			GLOBAL_SETTING_WRITE["trace"] = std::to_string(ProgramSettings::trace);
			GLOBAL_SETTING_WRITE["trace-level"] = std::to_string(ProgramSettings::trace_level);
			ini_file.generate(ini);
		}
	}

	void writeIni()
	{
		GLOBAL_SETTING_WRITE["tcp-ip"] = ProgramSettings::tcp_ip;
		GLOBAL_SETTING_WRITE["tcp-port"] = std::to_string(ProgramSettings::tcp_port);
		GLOBAL_SETTING_WRITE["trace"] = std::to_string(ProgramSettings::trace);
		GLOBAL_SETTING_WRITE["trace-level"] = std::to_string(ProgramSettings::trace_level);
		ini_file.write(ini);
		ProgramSettings::has_changed = false;
	}

	bool writeToIni(std::string& section, std::string& name, std::string& value)
	{
		ini[section.c_str()][name.c_str()] = value;
		return ini_file.write(ini);
	}
}
