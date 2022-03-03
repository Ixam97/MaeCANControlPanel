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
 * configworker.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-03.1]
 */

#include "configworker.h"
#include "globals.h"

void ConfigWorker::reset()
{
    uid = 0;
    m_current_index = 0;
    m_frameInVector.resize(0);
    busy = false;

}

void ConfigWorker::addFrameToQueue(canFrame _frame)
{
    if (global_states.tcp_success)
        m_frameOutQueue.push(_frame);
}

void ConfigWorker::addFrame(canFrame _frame)
{
    if (!busy) return;

    m_frameInVector.push_back(_frame);

    if (_frame.dlc == 6 || _frame.dlc == 5)
    {
        // Received channel completely, construct it.

        for (int i = 0; i < device_list.size(); i++)
        {
            if (device_list[i].uid == uid)
            {
                // Index 0, basic infos
                if (m_current_index == 0)
                {
                    device_list[i].num_config_channels = m_frameInVector[0].data[1];
                    device_list[i].num_readings_channels = m_frameInVector[0].data[0];
                    device_list[i].serialnbr = (m_frameInVector[0].data[4] << 24) | (m_frameInVector[0].data[5] << 16) | (m_frameInVector[0].data[6] << 8) | m_frameInVector[0].data[7];

                    for (int j = 0; j < 8; j++)
                    {
                        if (m_frameInVector[1].data[j] == 0) break;
                        device_list[i].item += (char)m_frameInVector[1].data[j];
                    }

                    for (int j = 2; j < m_frameInVector.size() - 1; j++)
                    {
                        for (int k = 0; k < 8; k++)
                        {
                            if (m_frameInVector[j].data[k] == 0) break;
                            device_list[i].name += (char)m_frameInVector[j].data[k];
                        }
                    }
                    device_list[i].name.resize(device_list[i].name.find_last_not_of(' ') + 1);
                    device_list[i].item.resize(device_list[i].item.find_last_not_of(' ') + 1);
                }

                // Other indexes are config or readings channels
                // Readings channels will be transmitted first
                else
                {
                    // Add new readings channel to device
                    if (m_current_index <= device_list[i].num_readings_channels)
                    {
                        readingsChannel new_readings_channel;

                        new_readings_channel.channel_index = m_frameInVector[0].data[0];
                        new_readings_channel.power = m_frameInVector[0].data[1];
                        new_readings_channel.colors[0] = m_frameInVector[0].data[2];
                        new_readings_channel.colors[1] = m_frameInVector[0].data[3];
                        new_readings_channel.colors[2] = m_frameInVector[0].data[4];
                        new_readings_channel.colors[3] = m_frameInVector[0].data[5];
                        new_readings_channel.points[0] = (m_frameInVector[0].data[6] << 8) | m_frameInVector[0].data[7];

                        new_readings_channel.points[1] = (m_frameInVector[1].data[0] << 8) | m_frameInVector[1].data[1];
                        new_readings_channel.points[2] = (m_frameInVector[1].data[2] << 8) | m_frameInVector[1].data[3];
                        new_readings_channel.points[3] = (m_frameInVector[1].data[4] << 8) | m_frameInVector[1].data[5];
                        new_readings_channel.points[4] = (m_frameInVector[1].data[6] << 8) | m_frameInVector[1].data[7];

                        int string_index = 0;

                        for (int j = 2; j < m_frameInVector.size() - 1; j++)
                        {
                            for (int k = 0; k < 8; k++)
                            {
                                if (m_frameInVector[j].data[k] == 0) string_index++;
                                else
                                {
                                    if (string_index == 0)
                                    {
                                        new_readings_channel.label += (char)m_frameInVector[j].data[k];
                                    }
                                    else if (string_index == 1)
                                    {
                                        new_readings_channel.s_min += (char)m_frameInVector[j].data[k];
                                    }
                                    else if (string_index == 2)
                                    {
                                        new_readings_channel.s_max += (char)m_frameInVector[j].data[k];
                                    }
                                    else if (string_index == 3)
                                    {
                                        new_readings_channel.unit += (char)m_frameInVector[j].data[k];
                                    }

                                }
                            }
                        }

                        if (new_readings_channel.label == "TRACK") new_readings_channel.label = "Gleisstrom";
                        else if (new_readings_channel.label == "VOLT") new_readings_channel.label = "Gleisspannung";
                        else if (new_readings_channel.label == "TEMP")
                        {
                            new_readings_channel.label = "Temperatur";
                            new_readings_channel.unit = u8"∞C";
                        }

                        new_readings_channel.value_factor = (std::stof(new_readings_channel.s_max) - std::stof(new_readings_channel.s_min)) / (new_readings_channel.points[4] - new_readings_channel.points[0]);
                        new_readings_channel.current_value = new_readings_channel.points[0];

                        device_list[i].vec_readings_channels.push_back(new_readings_channel);

                        // Add channel to request list
                        readingsRequestInfo new_readings_request = { device_list[i].uid, new_readings_channel.channel_index };
                        readings_request_list.push_back(new_readings_request);
                        global_states.new_request_list_entry = true;
                    }

                    // Add new config channel to device
                    else
                    {
                        configChannel new_config_channel;

                        new_config_channel.channel_index = m_frameInVector[0].data[0];
                        new_config_channel.type = m_frameInVector[0].data[1];

                        if (new_config_channel.type == 1)
                        {
                            new_config_channel.num_options = m_frameInVector[0].data[2];
                            new_config_channel.current_value = m_frameInVector[0].data[3];
                            // 4 Bytes reserved
                            int string_index = 0;
                            for (int j = 1; j < m_frameInVector.size() - 1; j++)
                            {
                                for (int k = 0; k < 8; k++)
                                {
                                    if (m_frameInVector[j].data[k] == 0) string_index++;
                                    else
                                    {
                                        if (string_index == 0)
                                        {
                                            new_config_channel.label += (char)m_frameInVector[j].data[k];
                                        }
                                        else
                                        {
                                            if (new_config_channel.dropdown_options.size() < string_index) new_config_channel.dropdown_options.resize(string_index);
                                            new_config_channel.dropdown_options[string_index - 1] += (char)m_frameInVector[j].data[k];

                                        }

                                    }
                                }
                            }
                            for (std::string str : new_config_channel.dropdown_options)
                            {
                                new_config_channel.dropdown_options_separated_by_zero += str;
                                new_config_channel.dropdown_options_separated_by_zero += '\0';
                            }
                        }

                        else if (new_config_channel.type == 2)
                        {
                            new_config_channel.min = (m_frameInVector[0].data[2] << 8) | m_frameInVector[0].data[3];
                            new_config_channel.max = (m_frameInVector[0].data[4] << 8) | m_frameInVector[0].data[5];
                            new_config_channel.current_value = (m_frameInVector[0].data[6] << 8) | m_frameInVector[0].data[7];

                            int string_index = 0;
                            for (int j = 1; j < m_frameInVector.size() - 1; j++)
                            {
                                for (int k = 0; k < 8; k++)
                                {
                                    if (m_frameInVector[j].data[k] == 0) string_index++;
                                    else
                                    {
                                        if (string_index == 0)
                                        {
                                            new_config_channel.label += (char)m_frameInVector[j].data[k];
                                        }
                                        else if (string_index == 1)
                                        {
                                            new_config_channel.s_min += (char)m_frameInVector[j].data[k];
                                        }
                                        else if (string_index == 2)
                                        {
                                            new_config_channel.s_max += (char)m_frameInVector[j].data[k];
                                        }
                                        else if (string_index == 3)
                                        {
                                            new_config_channel.unit += (char)m_frameInVector[j].data[k];
                                        }

                                    }
                                }
                            }
                        }
                        new_config_channel.wanted_value = new_config_channel.current_value;
                        device_list[i].vec_config_channels.push_back(new_config_channel);
                    }
                }

                // Reset frame vector and increment index
                m_frameInVector.resize(0);
                m_current_index++;

                // Request more config channels if they exist
                if (m_current_index <= (device_list[i].num_config_channels + device_list[i].num_readings_channels))
                {
                    uint8_t i_data[8] = { (uint8_t)(uid >> 24), (uint8_t)(uid >> 16),(uint8_t)(uid >> 8), (uint8_t)uid, m_current_index, 0,0,0 };
                    addFrameToQueue(newCanFrame(CMD_CONFIG, 0, 5, i_data));
                }

                // Done
                else
                {
                    busy = false;
                    device_list[i].data_complete = true;
                }
                break;
            }
        }
    }
}

bool ConfigWorker::getFrame(canFrame& _frame)
{
    if (m_frameOutQueue.size() == 0) return false;
    else {
        _frame = m_frameOutQueue.front();
        m_frameOutQueue.pop();
        return true;
    }
}

void ConfigWorker::workOn(uint32_t _uid)
{
    m_current_index = 0;
    busy = true;
    uid = _uid;

    uint8_t i_data[8] = { (uint8_t)(_uid >> 24), (uint8_t)(_uid >> 16),(uint8_t)(_uid >> 8), (uint8_t)_uid, m_current_index, 0,0,0 };
    addFrameToQueue(newCanFrame(CMD_CONFIG, 0, 5, i_data));

}
