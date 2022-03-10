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
 * Commit: [2022-03-10.1]
 */

#include "updater.h"
#include <fstream>
#include <filesystem>

int MCANUpdater::hexFileParser(std::string _file_name)
{
	std::ifstream hexfile;
	hexfile.open(_file_name);

	if (!hexfile.is_open())
	{
		Interface::logError("hexFileParser: Can't open File.");
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
			catch (...) { Interface::logError("hexFileParser: Parsing error."); hexfile.close(); return PARSE_ERR_CONVERSION; }
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
					catch (...) { Interface::logError("hexFileParser: Parsing error."); hexfile.close(); return PARSE_ERR_CONVERSION; }
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
				Interface::logError("hexFileParser: File is not supported");
				return PARSE_ERR_NOT_SUPPORTED;
				break;
			}
			char s_checksum[2];
			hexfile.get(s_checksum[0]);
			hexfile.get(s_checksum[1]);
			int checksum;
			try { checksum = std::stoi(s_checksum, NULL, 16); }
			catch (...) { Interface::logError("hexFileParser: Parsing error."); hexfile.close(); return PARSE_ERR_CONVERSION; }

			if ((-line_checksum & 0xff) != checksum && line_checksum > -1)
			{
				Interface::logError("hexFileParser: Checksum error.");
				hexfile.close();
				return PARSE_ERR_CHECKSUM;
			}
		}
	}

	hexfile.close();
	Interface::logInfo("hexFileParser: HEX file has been parsed successfull!");
	return 1;
}

bool MCANUpdater::busy()
{
	return m_busy;
}

float MCANUpdater::getProgress()
{
	if (m_busy) return m_progress;
	else return 0;
}

void MCANUpdater::addFrame(Interface::CanFrame& _frame)
{
	//m_frameInQueue.push(_frame);

	if (Interface::getUidFromData(_frame.data) != m_uid || _frame.resp == 0) {
		m_busy = false;
		Interface::logError("MCANUpdater: Detected conflicting update, abort.");
		UpdateInterface::status = MCAN_UPDATE_FAILURE_ERROR;
		return;
	}

	int return_code = MCAN_UPDATE_FAILURE_ERROR;

	/* 
	 * 0x01 : Offer/Accept Update (LEGACY)
	 * 0x11 : Offer/Accept Update
	 * 0x02 : Page Size (16Bit)
	 * 0x03 : Page count (16Bit)
	 * 0x04 : Begin Page (LEGACY, 8Bit page index)
	 * 0x14 : Begin Page (16Bit page index)
	 * 0x05 : End Page (LEGACY, 8Bit page index)
	 * 0x15 : End Page (16Bit page index)
	 * 0x06 : Abort
	 * 0x07 : Update completed
	 */

	switch (_frame.data[4]) {
	case 0x01:
		// LEGACY BOOTLOADER, Update offer acceppted
		if (_frame.dlc == 6) 
		{
			m_legacy = true;
			m_reported_type = _frame.data[5];
			return_code = MCAN_UPDATE_IN_PROGRESS;
			Interface::logInfo("MCANUpdater: Beginning update in Legacy Mode");
		}
		else if (_frame.dlc == 7)
		{
			if (_frame.data[6] > 0)
			{
				m_legacy = false;
				Interface::logInfo("MCANUpdater: Beginning update");
			}
			else
			{
				m_legacy = true;
				Interface::logInfo("MCANUpdater: Beginning update in Legacy Mode");
			}
			m_reported_type = _frame.data[5];
			return_code = MCAN_UPDATE_IN_PROGRESS;
		}
		break;
	case 0x11:
		// Update offer acceppted
		if (_frame.dlc == 6)
		{
			m_legacy = false;
			m_reported_type = _frame.data[5];
			return_code = MCAN_UPDATE_IN_PROGRESS;
		}
		break;
	case 0x02:
		if (_frame.dlc == 7)
		{
			m_page_size = (_frame.data[5] << 8) | _frame.data[6];
			return_code = MCAN_UPDATE_IN_PROGRESS;
		}
		break;
	case 0x03:
		if (_frame.dlc == 7)
		{
			m_page_count = (_frame.data[5] << 8) | _frame.data[6];
			return_code = MCAN_UPDATE_IN_PROGRESS;
		}
		break;
	case 0x04:
		break;
	case 0x05:
		// LEGACY BOOTLOADER, MAX 64kByte!
		if (_frame.dlc == 7)
		{
			if (_frame.data[6] == 0) 
			{
				// Error, retry page
				m_page_index = _frame.data[5];
				sendPage(m_page_index);
				return_code = MCAN_UPDATE_IN_PROGRESS;
			}
			else if (_frame.data[6] == 1)
			{
				m_page_index = _frame.data[5] + 1;
				if (m_page_index == m_file_page_count)
				{
					// Update completed
					uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid, 0x07, (uint8_t)m_type, 0,0 };
					m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 6, tmp_data));
					return_code = MCAN_UPDATE_SUCCESS;
					m_progress = 1.0f;
					m_busy = false;
				}
				else
				{
					// Send next page
					sendPage(m_page_index);
					m_progress = (float)m_page_index / m_file_page_count;
					return_code = MCAN_UPDATE_IN_PROGRESS;
				}
				
			}
			else return_code = MCAN_UPDATE_FAILURE_ERROR;			
		}
		break;
	case 0x15:
		if (_frame.dlc == 5)
		{
			// Error, retry page
			sendPage(m_page_index);
			return_code = MCAN_UPDATE_IN_PROGRESS;
		}
		else if (_frame.dlc == 7)
		{
			uint16_t returned_page_index = (_frame.data[5] << 8) | _frame.data[6];
			m_page_index = returned_page_index + 1; 
			if (m_page_index == m_file_page_count)
			{
				// Update completed
				uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid, 0x07, (uint8_t)m_type, 0,0 };
				m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 6, tmp_data));
				return_code = MCAN_UPDATE_SUCCESS;
				m_busy = false;
			}
			else
			{
				// Send next page
				sendPage(m_page_index);
				m_progress = (float)m_page_index / m_file_page_count;
				return_code = MCAN_UPDATE_IN_PROGRESS;
			}
		}
		else return_code = MCAN_UPDATE_FAILURE_ERROR;
		break;
	default:
		return_code = MCAN_UPDATE_FAILURE_INCOMPATIBLE;
		break;
	}

	// All Information available to start Update
	if (m_reported_type && m_page_count && m_page_size && !m_compatibility)
	{
		m_file_page_count = m_hexfile_byte_stream.size() / m_page_size + 1;

		if (m_type != m_reported_type) return_code = MCAN_UPDATE_FAILURE_INCOMPATIBLE;
		if (m_legacy && m_file_page_count > 0xff) return_code = MCAN_UPDATE_FAILURE_INCOMPATIBLE;
		if (m_page_count * m_page_size < m_hexfile_byte_stream.size()) return_code = MCAN_UPDATE_FAILURE_INCOMPATIBLE;

		if (return_code == MCAN_UPDATE_IN_PROGRESS) {
			// Confirmed compatibility, continue with update
			m_compatibility = true;
			uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid , 0x01, (uint8_t)m_type, 0x01,0 };
			m_frameOutQueue.push(Interface::CanFrame(0x40, 1, 7, tmp_data));
			sendPage(0);
		}
		else 
		{
			// Abort
			uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid , 0x01, (uint8_t)m_type, 0x00,0 };
			m_frameOutQueue.push(Interface::CanFrame(0x40, 1, 7, tmp_data));
		}
	}

	if (return_code == MCAN_UPDATE_FAILURE_ERROR)
	{
		uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid , 0x06, 0,0,0 };
		m_frameOutQueue.push(Interface::CanFrame(0x40, 1, 5, tmp_data));
		Interface::logError("MCANUpdater: Failed to do update.");
	}
	else if (return_code == MCAN_UPDATE_FAILURE_INCOMPATIBLE)
		Interface::logWarn("MCANUpdater: Detected incompatibility between update file and CAN device.");
	else if (return_code == MCAN_UPDATE_SUCCESS)
	{
		Interface::logInfo("MCANUpdater: Update successfull.");
		m_busy = false;
	}
	UpdateInterface::status = return_code;
}

void MCANUpdater::sendPage(int _index)
{
	if (m_legacy)
	{
		uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid, 0x04, (uint8_t)_index, 0,0 };
		m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 6, tmp_data));
	}
	else
	{
		uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid, 0x14, (uint8_t)(_index >> 8), (uint8_t)_index,0 };
		m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 7, tmp_data));
	}

	for (int i = 0; i < m_page_size / 8; i++)
	{
		uint8_t payload_data[8];
		for (int j = 0; j < 8; j++) 
		{
			if ((_index * m_page_size) + i * 8 + j >= m_hexfile_byte_stream.size()) payload_data[j] = 0xff;
			else payload_data[j] = m_hexfile_byte_stream[(_index * m_page_size) + i * 8 + j];
		}
		m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 8, payload_data, 0x300 + i + 1));
	}

	if (m_legacy)
	{
		uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid, 0x05, (uint8_t)_index, 0,0 };
		m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 6, tmp_data));
	}
	else
	{
		uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid, 0x15, (uint8_t)(_index >> 8), (uint8_t)_index,0 };
		m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 7, tmp_data));
	}

}

bool MCANUpdater::getFrame(Interface::CanFrame& _frame)
{
	if (m_frameOutQueue.size() == 0)
		return false;
	_frame = m_frameOutQueue.front();
	m_frameOutQueue.pop();
	return true;
}

void MCANUpdater::getFileNames(std::vector<std::string>& _file_names)
{
	std::vector<std::string> tmp_file_names;
	for (auto& file_name : std::filesystem::directory_iterator(std::string(".")))
	{
		std::string string_name = file_name.path().string().substr(file_name.path().string().find_last_of('\\') + 1);
		if (string_name.substr(string_name.find_last_of('.') + 1) == std::string("hex") || string_name.substr(string_name.find_last_of('.') + 1) == std::string("HEX"))
		{
			tmp_file_names.push_back(file_name.path().string());
		}
	}
	_file_names = tmp_file_names;
}

int MCANUpdater::startUpdate(uint32_t _uid, uint16_t _type, std::string _file_name)
{
	m_busy = true;
	m_uid = _uid;
	m_type = _type;

	m_page_index = 0;
	m_frame_index = 0;
	m_page_size = 0;
	m_page_count = 0;
	m_progress = 0;
	m_compatibility = false;

	hexFileParser(_file_name);

	uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid, 0,0,0,0 };
	m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 4, tmp_data));

	return MCAN_UPDATE_INIT;
}

void MCANUpdater::repeatUpdateOffer()
{
	uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid, 0,0,0,0 };
	m_frameOutQueue.push(Interface::CanFrame(0x40, 0, 4, tmp_data));
}

int MCANUpdater::abortUpdate()
{
	m_busy = false;
	m_page_index = 0;
	m_frame_index = 0;
	m_page_size = 0;
	m_page_count = 0;
	m_progress = 0;
	m_compatibility = false;

	uint8_t tmp_data[8] = { (uint8_t)(m_uid >> 24), (uint8_t)(m_uid >> 16), (uint8_t)(m_uid >> 8), (uint8_t)m_uid , 0x06, 0,0,0 };
	m_frameOutQueue.push(Interface::CanFrame(0x40, 1, 5, tmp_data));

	return MCAN_UPDATE_IDLE;
}
