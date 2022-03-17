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
 * Commit: [2022-03-17.1]
 */

#include "globals.h"
#include <vector>
#include <queue>

#pragma once
class ConfigWorker
{
private:
    static inline std::vector<Globals::CanFrame> m_frameInVector;
    static inline std::queue<Globals::CanFrame> m_frameOutQueue;
    static inline uint8_t m_current_index = 0;
    static inline bool m_busy = false;

    static void addFrameToQueue(Globals::CanFrame _frame);
	static void removeFromReadingsRequestList(uint32_t _uid);
public:
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

    inline static std::vector<canDevice> device_list;
    inline static std::vector<readingsRequestInfo> readings_request_list;

    static inline uint32_t uid = 0;

    static bool busy() { return m_busy; }

    // reset the config worker
    static void reset();

	static void removeFromDeviceList(size_t _index);

    // add CAN frame to internal input buffer and process it if ready
    static void addFrame(Globals::CanFrame _frame);

    // get CAN frame from internal output buffer
    static bool getFrame(Globals::CanFrame& _frame);

    // Define a device to get config from
    static void workOn(uint32_t _uid);
};

