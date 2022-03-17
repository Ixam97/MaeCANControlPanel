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
 * Commit: [2022-03-17.1]
 */

#pragma once
//#include "canframe.h"
#include "globals.h"
#include <vector>
#include <queue>
#include <string>

class MCANUpdater
{
private:
	inline static std::queue<Globals::CanFrame> m_frameInQueue;
	inline static std::queue<Globals::CanFrame> m_frameOutQueue;

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
	
	struct UpdateInterface
	{
		static inline uint32_t uid;
		static inline uint16_t type;
		static inline int status;
		static inline bool do_update = false;
		static inline bool do_abort = false;
		static inline bool get_file_names = false;
		static inline bool in_progress = false;
		static inline float progress = 0.0f;
		static inline std::vector<std::string> file_names;
		static inline std::string file_name;

	};
	

	// Get file names from file system
	static void getFileNames(std::vector<std::string>& _file_names);

	static int startUpdate(uint32_t _uid, uint16_t _type, std::string _file_name);

	static void repeatUpdateOffer();

	static int abortUpdate();

	// Return if updater is running.
	static bool busy();

	// Retun the current update progress.
	static float getProgress();

	// Pass a frame to the updater
	static void addFrame(Globals::CanFrame& _frame);

	// Get a frame the updater wants to send
	static bool getFrame(Globals::CanFrame& _frame);
};


