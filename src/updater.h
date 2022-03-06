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
 * updater.h
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-06.1]
 */

#pragma once

#include "canframe.h"
#include "globals.h"
#include <queue>

class MCANUpdater
{
private:
	inline static std::queue<canFrame> m_frameInQueue;
	inline static std::queue<canFrame> m_frameOutQueue;

	inline static std::vector<uint8_t> m_hexfile_byte_stream;

	inline static uint32_t m_uid;
	inline static uint16_t m_type;
	inline static int m_file_page_count;

	inline static int m_status;

	inline static bool m_legacy;
	inline static int m_page_index;
	inline static int m_frame_index;
	inline static int m_page_size;
	inline static int m_page_count;
	inline static uint16_t m_reported_type;
	inline static bool m_compatibility = false;

	inline static bool m_busy = false;
	inline static float m_progress = 0;

	static int hexFileParser(std::string _file_name);
	static void sendPage(int _index);

public:
	// Get file names from file system
	static void getFileNames(std::vector<std::string>& _file_names);

	static int startUpdate(uint32_t _uid, uint16_t _type, std::string _file_name);

	static int abortUpdate();

	// Return if updater is running.
	static bool busy();

	// Retun the current update progress.
	static float getProgress();

	// Pass a frame to the updater
	static int addFrame(canFrame& _frame);

	// Get a frame the updater wants to send
	static bool getFrame(canFrame& _frame);
};


