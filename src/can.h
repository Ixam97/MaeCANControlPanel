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
 * can.h
 * (c)2022 Maximilian Goldschmidt
 */

#pragma once
#define INQUEUE true
#define OUTQUEUE false

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdint.h>
#include <queue>
#include <vector>

#include "globals.h"
#include "canframe.h"

class CAN 
{
private:
    std::queue<canFrame> m_frameOutQueue;
    std::queue<canFrame> m_frameInQueue;
    long m_return_code = 0;
    SOCKET s;
    SOCKADDR_IN addr;

    std::string m_ip = "";
    int m_port = 0;

public:

    // Constructor
    // CAN();

    // Init a TCP connection
    bool TCPConnect();

    // Check for successfull connection
    void TCPCheckConnection();

    // Disconnect TCP connection
    void TCPDisconnect();

    // Reads a frame from the TCP socket and adds it to INQUEUE
    int TCPReadFrame();

    // Process the queue of CAN frames to be sent
    // bool _b_recque: Determine which queue to process. OUTQUEUE or INQUEUE 
    // Returns the oldest canFrame from the queue
    canFrame processQueue(bool _b_recqueue);

    // Add a CAN fram to the queue
    // canFrame _frame : Can frame to be added
    // bool _b_recframe: OUTQUEUE or INQUEUE
    int addFrameToQueue(canFrame _frame, bool _b_recframe);

    size_t getQueueLength(bool _b_recqueue);
};