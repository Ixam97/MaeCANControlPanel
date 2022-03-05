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
 * Commit: [2022-03-05.1]
 */

#pragma once

#define PARSE_ERR_FILE_OPEN -1
#define PARSE_ERR_CHECKSUM -2
#define PARSE_ERR_CONVERSION -3
#define PARSE_ERR_NOT_SUPPORTED -4

#include "globals.h"
#include "canframe.h"
#include <queue>

class MCANUpdater
{
private:
	inline static std::queue<canFrame> m_frameInQueue;
	inline static std::queue<canFrame> m_frameOutQueue;

	inline static std::vector<std::string> m_file_names;

	inline static std::vector<uint8_t> m_hexfile_byte_stream;

	inline static bool m_busy = false;

	static int hexFileParser(std::string _file_name);
	static void getFileNames();

public:
	// Return if updater is running.
	static bool busy();

	// Pass a frame to the updater
	static void addFrame(canFrame& _frame);

	// Get a frame the updater wants to send
	static bool getFrame(canFrame& _frame);
};


