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
 * can.cpp
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-10.1]
 */

#include "can.h"

bool CAN::TCPConnect()
{
    WSADATA wsa;
    m_return_code = WSAStartup(MAKEWORD(2, 0), &wsa);

    if (m_return_code != 0) {
        Interface::logError("Init: startWinsock failed.", m_return_code);
        Interface::ProgramStates::tcp_error_code = m_return_code;
        return false;
    }

    // Create TCP socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        int iErrorCode = WSAGetLastError();
        Interface::logError("Init: Could not create socket.", iErrorCode);
        Interface::ProgramStates::tcp_error_code = iErrorCode;
        return false;
    }
    u_long iMode = 1;
    if (ioctlsocket(s, FIONBIO, &iMode) != NO_ERROR)
    {
        Interface::logError("Init: Could not enter non blocking mode.");
        return false;
    }

    // Connect TCP socket

    m_port = Interface::ProgramSettings::tcp_port;
    m_ip = Interface::ProgramSettings::tcp_ip;

    addr.sin_port = htons(m_port);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, (PCSTR)m_ip.c_str(), &addr.sin_addr.s_addr);

    m_return_code = connect(s, (SOCKADDR*)&addr, sizeof(SOCKADDR));

    if (m_return_code == SOCKET_ERROR) 
    {
        int iErrorCode = WSAGetLastError();
        
        if (iErrorCode == WSAEWOULDBLOCK)
        {
            Interface::logWarn("TCPConnect: Connection in Progress.", iErrorCode);
            Interface::ProgramStates::tcp_error_code = iErrorCode;
            return true;
        }
        Interface::logError("TCPConnect: Connect failed.", iErrorCode);
        return false;
    }
    Interface::logInfo("TCPConnect: Connected.");
    uint8_t stopData[] = { 0,0,0,0,0,0,0,0 };
    Interface::CanFrame stopFrame(0x00000300, 5, stopData);
    addFrameToQueue(stopFrame, OUTQUEUE);
    return true;
}

void CAN::TCPCheckConnection()
{
    // Check for lost connection
    if (Interface::ProgramStates::tcp_success)
    {
        char dummy_buff[1];
        m_return_code = recv(s, dummy_buff, 0,0);
        if (m_return_code == SOCKET_ERROR)
        {
            int iErrorCode = WSAGetLastError();
            if (iErrorCode == WSAECONNRESET)
            {
                Interface::logError("TCPCheck: Lost TCP connection.", iErrorCode);
                Interface::ProgramStates::tcp_success = false;
                Interface::ProgramStates::tcp_started = false;
                Interface::ProgramStates::tcp_error_code = iErrorCode;
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
                Interface::ProgramStates::tcp_success = true;
                Interface::ProgramStates::connected_ip = m_ip;
                Interface::ProgramStates::connected_port = m_port;
                Interface::ProgramStates::tcp_started = false;
                Interface::ProgramStates::tcp_error_code = 0;
                Interface::logInfo("TCPCheck: Connected.");
                uint8_t stopData[] = { 0,0,0,0,0,0,0,0 };
                Interface::CanFrame stopFrame(0x00000300, 5, stopData);
                addFrameToQueue(stopFrame, OUTQUEUE);
                return;
            }
            else if (iErrorCode == WSAEALREADY)
            {
                Interface::ProgramStates::tcp_error_code = iErrorCode;
                return;
            }
            else if (iErrorCode != WSAEALREADY)
            {
                Interface::ProgramStates::tcp_started = false;
                Interface::ProgramStates::tcp_error_code = iErrorCode;
                Interface::logError("TCPCheck: Connect failed.", iErrorCode);
            }
        }
        else
        {
            Interface::ProgramStates::tcp_success = true;
            Interface::ProgramStates::connected_ip = m_ip;
            Interface::ProgramStates::connected_port = m_port;
            Interface::ProgramStates::tcp_started = false;
            Interface::ProgramStates::tcp_error_code = 0;
            Interface::logInfo("TCPCheck: Connected.");
            uint8_t stopData[] = { 0,0,0,0,0,0,0,0 };
            Interface::CanFrame stopFrame(0x00000300, 5, stopData);
            addFrameToQueue(stopFrame, OUTQUEUE);
        }
    }
}

void CAN::TCPDisconnect()
{
    char datagram[13] = { 0,0,0x03,0x00,5,0,0,0,0,0,0,0,0 };
    m_return_code = send(s, datagram, 13, 0);
    if (m_return_code == SOCKET_ERROR)
    {
        int iErrorCode = WSAGetLastError();
        Interface::logError("processQueue: Frame could not be sent.", iErrorCode);
    }

    m_return_code = closesocket(s);
    if (m_return_code != SOCKET_ERROR)
    {
        Interface::ProgramStates::tcp_success = false;
        Interface::ProgramStates::tcp_started = false;
        Interface::ProgramStates::tcp_error_code = 0;
        Interface::logInfo("TCPDisconnect: Disconnected.");
    }
    
}

int CAN::addFrameToQueue(Interface::CanFrame _frame, bool _b_recframe)
{
    if (_frame.can_hash & 0x300 || (_frame.cmd == CMD_CONFIG && _frame.resp == 1))
    {
        if (_b_recframe == INQUEUE)
            m_frameInQueue.push(_frame);
        else if (Interface::ProgramStates::tcp_success)
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
    if (Interface::ProgramStates::tcp_success)
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
                Interface::CanFrame _recFrame(((uint8_t)_buff[0] << 24) | ((uint8_t)_buff[1] << 16) | ((uint8_t)_buff[2] << 8) | (uint8_t)_buff[3], (uint8_t)_buff[4], _data);

                addFrameToQueue(_recFrame, INQUEUE);

                frame_count++;
            }
            return frame_count;
        }
    }
    
    return 0;
}

Interface::CanFrame CAN::processQueue(bool _b_recqueue)
{
    Interface::CanFrame queuedFrame;
    if (Interface::ProgramStates::tcp_success)
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
            char datagram[] = {(char)(queuedFrame.id >> 24), (char)(queuedFrame.id >> 16), (char)(queuedFrame.id >> 8), (char)(queuedFrame.id), (char)queuedFrame.dlc, 
            (char)queuedFrame.data[0], (char)queuedFrame.data[1], (char)queuedFrame.data[2], (char)queuedFrame.data[3], (char)queuedFrame.data[4], (char)queuedFrame.data[5], (char)queuedFrame.data[6], (char)queuedFrame.data[7]};

            m_return_code = send(s, datagram, 13, 0);
            if (m_return_code == SOCKET_ERROR)
            {
                int iErrorCode = WSAGetLastError();
                Interface::logError("processQueue: Frame could not be sent.", iErrorCode);
            }                
            m_frameOutQueue.pop();
        }
    }
    return queuedFrame;
}

size_t CAN::getQueueLength(bool _b_recqueue)
{
    if (_b_recqueue == INQUEUE) return m_frameInQueue.size();
    else return (int)m_frameOutQueue.size();
}
