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
 * Commit: [2022-03-17.1]
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

class CAN 
{
private:
    static inline std::queue<Globals::CanFrame> m_frameOutQueue;
    static inline std::queue<Globals::CanFrame> m_frameInQueue;
    static inline long m_return_code = 0;
    static inline SOCKET s;
    static inline SOCKADDR_IN addr;

    static inline std::string m_ip = "";
    static inline int m_port = 0;

public:

    // Constructor
    // CAN();

    // Init a TCP connection
    static bool TCPConnect();

    // Check for successfull connection
    static void TCPCheckConnection();

    // Disconnect TCP connection
    static void TCPDisconnect();

    // Reads a frame from the TCP socket and adds it to INQUEUE
    static int TCPReadFrame();

    // Process the queue of CAN frames to be sent
    // bool _b_recque: Determine which queue to process. OUTQUEUE or INQUEUE 
    // Returns the oldest CanFrame from the queue
    static Globals::CanFrame processQueue(bool _b_recqueue);

    // Add a CAN fram to the queue
    // CanFrame _frame : Can frame to be added
    // bool _b_recframe: OUTQUEUE or INQUEUE
    static int addFrameToQueue(Globals::CanFrame _frame, bool _b_recframe);

    static size_t getQueueLength(bool _b_recqueue);
};
