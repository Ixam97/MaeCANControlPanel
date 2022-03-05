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
 * updater.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-05.1]
 */

#include "updater.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>

int MCANUpdater::hexFileParser(std::string _file_name)
{
	std::ifstream hexfile;
	hexfile.open(_file_name);

	if (!hexfile.is_open())
	{
		logError("hexFileParser: Can't open File.");
		return PARSE_ERR_FILE_OPEN;
	}

	m_hexfile_byte_stream.resize(0);

	char ch;
	while (hexfile.get(ch))
	{
		static int line_counter = 0;
		if (ch == ':') // New Line
		{
			line_counter++;
			int line_checksum = -1;

			char s_byte_count[2];
			hexfile.get(s_byte_count[0]);
			hexfile.get(s_byte_count[1]);
			int byte_count;

			char s_data_address[4];
			hexfile.get(s_data_address[0]);
			hexfile.get(s_data_address[1]);
			hexfile.get(s_data_address[2]);
			hexfile.get(s_data_address[3]);
			int data_address;

			char s_line_type[2];
			hexfile.get(s_line_type[0]);
			hexfile.get(s_line_type[1]);
			int line_type;

			try 
			{
				byte_count = std::stoi(s_byte_count, NULL, 16);
				data_address = std::stoi(s_data_address, NULL, 16);
				line_type = std::stoi(s_line_type, NULL, 16);
			}
			catch (...) { logError("hexFileParser: Parsing error."); hexfile.close(); return PARSE_ERR_CONVERSION; }
			line_checksum = byte_count + (data_address & 0xff) + ((data_address >> 8) & 0xff) + line_type;

			switch (line_type)
			{
			case 0x00:
				// Data 
				if (m_hexfile_byte_stream.size() < data_address) m_hexfile_byte_stream.resize(data_address, 0xff);
				for (int i = 0; i < byte_count; i++)
				{
					char s_data_byte[2];
					hexfile.get(s_data_byte[0]);
					hexfile.get(s_data_byte[1]);
					uint8_t data_byte;
					try { data_byte = (uint8_t)std::stoi(s_data_byte, NULL, 16); }
					catch (...) { logError("hexFileParser: Parsing error."); hexfile.close(); return PARSE_ERR_CONVERSION; }
					line_checksum += data_byte;
					m_hexfile_byte_stream.push_back(data_byte);
				}
				//std::cout << std::endl;
				break;
			case 0x01:
				// End of file
				break;
			case 0x02:
				// Extended segment address
			case 0x04:
				// Extended linear address
			default: 
				line_checksum = -1;
				logError("hexFileParser: File is not supported");
				return PARSE_ERR_NOT_SUPPORTED;
				break;
			}
			char s_checksum[2];
			hexfile.get(s_checksum[0]);
			hexfile.get(s_checksum[1]);
			int checksum;
			try { checksum = std::stoi(s_checksum, NULL, 16); }
			catch (...) { logError("hexFileParser: Parsing error."); hexfile.close(); return PARSE_ERR_CONVERSION; }

			if ((-line_checksum & 0xff) != checksum && line_checksum > -1)
			{
				logError("hexFileParser: Checksum error.");
				hexfile.close();
				return PARSE_ERR_CHECKSUM;
			}
		}
	}

	hexfile.close();
	logInfo("hexFileParser: HEX file has been parsed successfull!");
	return 1;
}

bool MCANUpdater::busy()
{
	return m_busy;
}

void MCANUpdater::addFrame(canFrame& _frame)
{

}

bool MCANUpdater::getFrame(canFrame& _frame)
{
	if (m_frameOutQueue.size() == 0)
		return false;
	_frame = m_frameOutQueue.front();
	m_frameOutQueue.pop();
	return true;
}

void MCANUpdater::getFileNames()
{
	for (auto& file_name : std::filesystem::directory_iterator(std::string(".")))
	{
		std::string string_name = file_name.path().string().substr(file_name.path().string().find_last_of('\\') + 1);
		if (string_name.substr(string_name.find_last_of('.') + 1) == std::string("hex") || string_name.substr(string_name.find_last_of('.') + 1) == std::string("HEX"))
		{
			m_file_names.push_back(file_name.path().string());

		}
	}

	for (std::string& file_name : m_file_names)
	{
		std::cout << file_name << std::endl;
	}
}
