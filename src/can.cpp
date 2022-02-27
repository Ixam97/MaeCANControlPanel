#include "can.h"

canFrame::canFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash)
{
    cmd = _cmd;
    resp = _resp;
    dlc = _dlc;
    can_hash = _hash;
    memcpy(data, _data, 8);
    id = (cmd << 17) | ((_resp & 1) << 16) | _hash;
}

canFrame::canFrame(uint32_t _id, uint8_t _dlc, uint8_t _data[8])
{
    id = _id;
    dlc = _dlc;
    memcpy(data, _data, 8);
    cmd = (uint8_t)(id >> 17);
    resp = (uint8_t)(id >> 16) & 0b1;
    can_hash = (uint16_t)id & 0xffff;
}

canFrame::canFrame()
{
}

canFrame newCanFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash)
{
    canFrame outFrame(_cmd, _resp, _dlc, _data, _hash);
    return outFrame;
}

canFrame newCanFrame(uint8_t _cmd, uint8_t _resp, uint16_t _hash)
{
    uint8_t out_data[] = { 0,0,0,0,0,0,0,0 };
    canFrame outFrame(_cmd, _resp, 0, out_data);
    return outFrame;
}

CAN::CAN()
{
    
}

bool CAN::TCPConnect()
{
    WSADATA wsa;
    m_return_code = WSAStartup(MAKEWORD(2, 0), &wsa);

    if (m_return_code != 0) {
        logError("Init: startWinsock failed.", m_return_code);
        global_states.tcp_error_code = m_return_code;
        return false;
    }

    // Create TCP socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        int iErrorCode = WSAGetLastError();
        logError("Init: Could not create socket.", iErrorCode);
        global_states.tcp_error_code = iErrorCode;
        return false;
    }
    u_long iMode = 1;
    if (ioctlsocket(s, FIONBIO, &iMode) != NO_ERROR)
    {
        logError("Init: Could not enter non blocking mode.");
        return false;
    }

    // Connect TCP socket

    m_port = global_settings.tcp_port;
    m_ip = global_settings.tcp_ip;

    addr.sin_port = htons(m_port);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, (PCSTR)m_ip.c_str(), &addr.sin_addr.s_addr);

    m_return_code = connect(s, (SOCKADDR*)&addr, sizeof(SOCKADDR));

    if (m_return_code == SOCKET_ERROR) 
    {
        int iErrorCode = WSAGetLastError();
        
        if (iErrorCode == WSAEWOULDBLOCK)
        {
            logWarn("TCPConnect: Connection in Progress.", iErrorCode);
            global_states.tcp_error_code = iErrorCode;
            return true;
        }
        logError("TCPConnect: Connect failed.", iErrorCode);
        return false;
    }
    logInfo("TCPConnect: Connected.");
    uint8_t stopData[] = { 0,0,0,0,0,0,0,0 };
    canFrame stopFrame(0x00000300, 5, stopData);
    addFrameToQueue(stopFrame, OUTQUEUE);
    return true;
}

void CAN::TCPCheckConnection()
{
    // Check for lost connection
    if (global_states.tcp_success)
    {
        char dummy_buff[1];
        m_return_code = recv(s, dummy_buff, 0,0);
        if (m_return_code == SOCKET_ERROR)
        {
            int iErrorCode = WSAGetLastError();
            if (iErrorCode == WSAECONNRESET)
            {
                logError("TCPCheck: Lost TCP connection.", iErrorCode);
                global_states.tcp_success = false;
                global_states.tcp_started = false;
                global_states.tcp_error_code = iErrorCode;
                closesocket(s);
                return;
            }
        }
    }

    // Check connection establishment
    else
    {
        m_return_code = connect(s, (SOCKADDR*)&addr, sizeof(SOCKADDR));
        if (m_return_code == SOCKET_ERROR)
        {
            int iErrorCode = WSAGetLastError();
            if (iErrorCode == WSAEISCONN)
            {
                global_states.tcp_success = true;
                global_states.connected_ip = m_ip;
                global_states.connected_port = m_port;
                global_states.tcp_started = false;
                global_states.tcp_error_code = 0;
                logInfo("TCPCheck: Connected.");
                uint8_t stopData[] = { 0,0,0,0,0,0,0,0 };
                canFrame stopFrame(0x00000300, 5, stopData);
                addFrameToQueue(stopFrame, OUTQUEUE);
                return;
            }
            else if (iErrorCode == WSAEALREADY)
            {
                global_states.tcp_error_code = iErrorCode;
                return;
            }
            else if (iErrorCode != WSAEALREADY)
            {
                global_states.tcp_started = false;
                global_states.tcp_error_code = iErrorCode;
                logError("TCPCheck: Connect failed.", iErrorCode);
            }
        }
        else
        {
            global_states.tcp_success = true;
            global_states.connected_ip = m_ip;
            global_states.connected_port = m_port;
            global_states.tcp_started = false;
            global_states.tcp_error_code = 0;
            logInfo("TCPCheck: Connected.");
            uint8_t stopData[] = { 0,0,0,0,0,0,0,0 };
            canFrame stopFrame(0x00000300, 5, stopData);
            addFrameToQueue(stopFrame, OUTQUEUE);
        }
    }
}

void CAN::TCPDisconnect()
{
    m_return_code = closesocket(s);
    if (m_return_code != SOCKET_ERROR)
    {
        global_states.tcp_success = false;
        global_states.tcp_started = false;
        global_states.tcp_error_code = 0;
        logInfo("TCPDisconnect: Disconnected.");
    }
    
}

int CAN::addFrameToQueue(canFrame _frame, bool _b_recframe)
{
    if (_frame.can_hash & 0x300 || (_frame.cmd == CMD_CONFIG && _frame.resp == 1))
    {
        if (_b_recframe == INQUEUE)
            m_frameInQueue.push(_frame);
        else if (global_states.tcp_success)
            m_frameOutQueue.push(_frame);
        return 1;
    }
    else
    {
        // Wrong Hash format
        return 0;
    }
}

int CAN::TCPReadFrame()
{
    if (global_states.tcp_success)
    {
        char _buff[13];

        struct timeval timeout = { 0, 1000 };
        struct fd_set readSet;

        FD_ZERO(&readSet);
        FD_SET(s, &readSet);

        int frame_count = 0;

        if (select(0, &readSet, 0, 0, &timeout))
        {
            while (recv(s, _buff, 13, 0) != SOCKET_ERROR)
            {
                uint8_t _data[13];
                memcpy(_data, &_buff[5], 8);
                canFrame _recFrame(((uint8_t)_buff[0] << 24) | ((uint8_t)_buff[1] << 16) | ((uint8_t)_buff[2] << 8) | (uint8_t)_buff[3], (uint8_t)_buff[4], _data);

                addFrameToQueue(_recFrame, INQUEUE);

                frame_count++;
            }
            return frame_count;
        }
    }
    
    return 0;
}

canFrame CAN::processQueue(bool _b_recqueue) 
{
    canFrame queuedFrame;
    if (global_states.tcp_success)
    {
        if (_b_recqueue == INQUEUE)
        {
            queuedFrame = m_frameInQueue.front();
            //printf("0x%08X [%d] %02X %02X %02X %02X %02X %02X %02X %02X\n", queuedFrame.id, queuedFrame.dlc, queuedFrame.data[0], queuedFrame.data[1], queuedFrame.data[2], queuedFrame.data[3], queuedFrame.data[4], queuedFrame.data[5], queuedFrame.data[6], queuedFrame.data[7]);
            m_frameInQueue.pop();
        }
        else
        {
            queuedFrame = m_frameOutQueue.front();
            char datagram[] = {(uint8_t)(queuedFrame.id >> 24), (uint8_t)(queuedFrame.id >> 16), (uint8_t)(queuedFrame.id >> 8), (uint8_t)(queuedFrame.id), queuedFrame.dlc, 
            queuedFrame.data[0], queuedFrame.data[1], queuedFrame.data[2], queuedFrame.data[3], queuedFrame.data[4], queuedFrame.data[5], queuedFrame.data[6], queuedFrame.data[7]};

            m_return_code = send(s, datagram, 13, 0);
            if (m_return_code == SOCKET_ERROR)
            {
                int iErrorCode = WSAGetLastError();
                logError("processQueue: Frame could not be sent.", iErrorCode);
            }                
            m_frameOutQueue.pop();
        }
    }
    return queuedFrame;
}

int CAN::getQueueLength(bool _b_recqueue)
{
    if (_b_recqueue == INQUEUE) return m_frameInQueue.size();
    else return (int)m_frameOutQueue.size();
}

ConfigWorker::ConfigWorker(CAN* _can)
{
    m_can = _can;
}

ConfigWorker::ConfigWorker(){}

void ConfigWorker::reset()
{
    uid = 0;
    m_current_index = 0;
    m_frame_vector.resize(0);
    busy = false;

}

void ConfigWorker::addFrame(canFrame _frame)
{
    if (!busy) return;

    m_frame_vector.push_back(_frame);

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
                    device_list[i].num_config_channels = m_frame_vector[0].data[1];
                    device_list[i].num_readings_channels = m_frame_vector[0].data[0];
                    device_list[i].serialnbr = (m_frame_vector[0].data[4] << 24) | (m_frame_vector[0].data[5] << 16) | (m_frame_vector[0].data[6] << 8) | m_frame_vector[0].data[7];
                    
                    for (int j = 0; j < 8; j++)
                    {
                        if (m_frame_vector[1].data[j] == 0) break;
                        device_list[i].item += (char)m_frame_vector[1].data[j];
                    }
                    
                    for (int j = 2; j < m_frame_vector.size() - 1; j++)
                    {
                        for (int k = 0; k < 8; k++)
                        {
                            if (m_frame_vector[j].data[k] == 0) break;
                            device_list[i].name += (char)m_frame_vector[j].data[k];
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

                        new_readings_channel.channel_index = m_frame_vector[0].data[0];
                        new_readings_channel.power = m_frame_vector[0].data[1];
                        new_readings_channel.colors[0] = m_frame_vector[0].data[2];
                        new_readings_channel.colors[1] = m_frame_vector[0].data[3];
                        new_readings_channel.colors[2] = m_frame_vector[0].data[4];
                        new_readings_channel.colors[3] = m_frame_vector[0].data[5];
                        new_readings_channel.points[0] = (m_frame_vector[0].data[6] << 8) | m_frame_vector[0].data[7];

                        new_readings_channel.points[1] = (m_frame_vector[1].data[0] << 8) | m_frame_vector[1].data[1];
                        new_readings_channel.points[2] = (m_frame_vector[1].data[2] << 8) | m_frame_vector[1].data[3];
                        new_readings_channel.points[3] = (m_frame_vector[1].data[4] << 8) | m_frame_vector[1].data[5];
                        new_readings_channel.points[4] = (m_frame_vector[1].data[6] << 8) | m_frame_vector[1].data[7];

                        int string_index = 0;

                        for (int j = 2; j < m_frame_vector.size() - 1; j++)
                        {
                            for (int k = 0; k < 8; k++)
                            {
                                if (m_frame_vector[j].data[k] == 0) string_index++;
                                else
                                {
                                    if (string_index == 0)
                                    {
                                        new_readings_channel.label += (char)m_frame_vector[j].data[k];
                                    }
                                    else if (string_index == 1)
                                    {
                                        new_readings_channel.s_min += (char)m_frame_vector[j].data[k];
                                    }
                                    else if (string_index == 2)
                                    {
                                        new_readings_channel.s_max += (char)m_frame_vector[j].data[k];
                                    }
                                    else if (string_index == 3)
                                    {
                                        new_readings_channel.unit += (char)m_frame_vector[j].data[k];
                                    }
                                    
                                }
                            }
                        }

                        if (new_readings_channel.label == "TRACK") new_readings_channel.label = "Gleisstrom";
                        else if (new_readings_channel.label == "VOLT") new_readings_channel.label = "Gleisspannung";
                        else if (new_readings_channel.label == "TEMP")
                        {
                            new_readings_channel.label = "Temperatur";
                            new_readings_channel.unit = u8"°C";
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

                        new_config_channel.channel_index = m_frame_vector[0].data[0];
                        new_config_channel.type = m_frame_vector[0].data[1];

                        if (new_config_channel.type == 1)
                        {
                            new_config_channel.num_options = m_frame_vector[0].data[2];
                            new_config_channel.current_value = m_frame_vector[0].data[3];
                            // 4 Bytes reserved
                            int string_index = 0;
                            for (int j = 1; j < m_frame_vector.size() - 1; j++)
                            {
                                for (int k = 0; k < 8; k++)
                                {
                                    if (m_frame_vector[j].data[k] == 0) string_index++;
                                    else
                                    {
                                        if (string_index == 0)
                                        {
                                            new_config_channel.label += (char)m_frame_vector[j].data[k];
                                        }
                                        else
                                        {
                                            if (new_config_channel.dropdown_options.size() < string_index) new_config_channel.dropdown_options.resize(string_index);
                                            new_config_channel.dropdown_options[string_index - 1] += (char)m_frame_vector[j].data[k];

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
                            new_config_channel.min = (m_frame_vector[0].data[2] << 8) | m_frame_vector[0].data[3];
                            new_config_channel.max = (m_frame_vector[0].data[4] << 8) | m_frame_vector[0].data[5];
                            new_config_channel.current_value = (m_frame_vector[0].data[6] << 8) | m_frame_vector[0].data[7];

                            int string_index = 0;
                            for (int j = 1; j < m_frame_vector.size() - 1; j++)
                            {
                                for (int k = 0; k < 8; k++)
                                {
                                    if (m_frame_vector[j].data[k] == 0) string_index++;
                                    else
                                    {
                                        if (string_index == 0)
                                        {
                                            new_config_channel.label += (char)m_frame_vector[j].data[k];
                                        }
                                        else if (string_index == 1)
                                        {
                                            new_config_channel.s_min += (char)m_frame_vector[j].data[k];
                                        }
                                        else if (string_index == 2)
                                        {
                                            new_config_channel.s_max += (char)m_frame_vector[j].data[k];
                                        }
                                        else if (string_index == 3)
                                        {
                                            new_config_channel.unit += (char)m_frame_vector[j].data[k];
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
                m_frame_vector.resize(0);
                m_current_index++;

                // Request more config channels if they exist
                if (m_current_index <= (device_list[i].num_config_channels + device_list[i].num_readings_channels))
                {
                    uint8_t i_data[8] = { (uint8_t)(uid >> 24), (uint8_t)(uid >> 16),(uint8_t)(uid >> 8), (uint8_t)uid, m_current_index, 0,0,0 };
                    m_can->addFrameToQueue(newCanFrame(CMD_CONFIG, 0, 5, i_data), OUTQUEUE);
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

void ConfigWorker::workOn(uint32_t _uid)
{
    m_current_index = 0;
    busy = true;
    uid = _uid;

    uint8_t i_data[8] = { (uint8_t)(_uid >> 24), (uint8_t)(_uid >> 16),(uint8_t)(_uid >> 8), (uint8_t)_uid, m_current_index, 0,0,0 };
    m_can->addFrameToQueue(newCanFrame(CMD_CONFIG, 0, 5, i_data), OUTQUEUE);
    
}