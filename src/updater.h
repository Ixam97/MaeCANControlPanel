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
 * Commit: [2022-03-03.1]
 */

#pragma once

#include "globals.h"
#include "canframe.h"
#include <fstream>
#include <iostream>
#include <filesystem>



class MCANUpdater
{
private:
	inline static std::ifstream hexfile;
	inline static std::vector<std::string> file_names;
public:
	static void getFileNames()
	{
		for (auto& file_name : std::filesystem::directory_iterator(std::string(".")))
		{
			std::string string_name = file_name.path().string().substr(file_name.path().string().find_last_of('\\') + 1);
			if (string_name.substr(string_name.find_last_of('.') + 1) == std::string("hex") || string_name.substr(string_name.find_last_of('.') + 1) == std::string("HEX"))
			{
				file_names.push_back(file_name.path().string());
				
			}
		}

		for (std::string & file_name : file_names)
		{
			std::cout << file_name << std::endl;
		}
	}
};


