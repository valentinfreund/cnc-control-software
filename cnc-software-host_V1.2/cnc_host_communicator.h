#ifndef CNC_HOST_COMMUNICATOR_H
#define CNC_HOST_COMMUNICATOR_H

#include "cnc_host_protocol.h"
#include <deque>
#include <mutex>
#include <atomic>
#include <string>
#include <thread>

class CNCHostCommunicator {
public:
    CNCHostCommunicator();
    ~CNCHostCommunicator();

    bool InitializeSPI(const std::string& device_path);
    void AddSegment(SPI_Poly_Segment_t seg);
    
    void SendPause();
    void SendResume();
    void SendEmergencyStop();
    void SendSystemCommand(uint8_t cmd_flag);
    void ClearQueue();
    
    uint32_t GetNextSegmentID();
    SPI_Status_Response_t GetLastStatus();

    void StartProcessingThread();
    void StopProcessingThread();
    void ProcessingLoop();

private:
    int spi_fd;
    uint32_t current_segment_id;
    
    std::deque<SPI_Poly_Segment_t> transmit_queue;
    std::mutex queue_mutex;
    
    SPI_Status_Response_t last_response;
    std::mutex status_mutex;

    std::atomic<bool> keep_running{false};
    std::thread worker_thread;
    std::atomic<bool> is_paused{false};

    uint16_t CalculateCRC16(const uint8_t* data, size_t length);
};

#endif CNC_HOST_COMMUNICATOR_H