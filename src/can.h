#pragma once
#define INQUEUE true
#define OUTQUEUE false

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <queue>
#include <vector>

#include "globals.h"
#include "candefs.h"

struct canFrame {
    canFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash = 0x300);
    canFrame(uint32_t _id, uint8_t _dlc, uint8_t _data[8]);
    canFrame();
    uint32_t id = 0;
    uint8_t cmd = 0;
    uint8_t resp = 0;
    uint16_t can_hash = 0;
    uint8_t dlc = 0;
    uint8_t data[8] = { 0,0,0,0,0,0,0,0 };
};

canFrame newCanFrame(uint8_t _cmd, uint8_t _resp, uint8_t _dlc, uint8_t _data[8], uint16_t _hash = 0x300);
canFrame newCanFrame(uint8_t _cmd, uint8_t _resp, uint16_t _hash = 0x300);

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
    CAN();

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

    int getQueueLength(bool _b_recqueue);
};

class ConfigWorker 
{
private:
    std::vector<canFrame> m_frame_vector;
    int m_current_index = 0;
    CAN* m_can;
public:
    ConfigWorker();
    ConfigWorker(CAN* _can);
    uint32_t uid = 0;
    bool busy = false;
    void reset();
    void addFrame(canFrame _frame);
    void workOn(uint32_t _uid);
};