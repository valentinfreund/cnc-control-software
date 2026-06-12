#include "cnc_host_communicator.h"
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

CNCHostCommunicator::CNCHostCommunicator() : spi_fd(-1), current_segment_id(1) {
    std::memset(&last_response, 0, sizeof(SPI_Status_Response_t));
}

CNCHostCommunicator::~CNCHostCommunicator() {
    StopProcessingThread();
    if (spi_fd >= 0) close(spi_fd);
}

bool CNCHostCommunicator::InitializeSPI(const std::string& device_path) {
    //not yet implemented
    return true;
}

void CNCHostCommunicator::StartProcessingThread() {
    if (!keep_running) {
        keep_running = true;
        worker_thread = std::thread(&CNCHostCommunicator::ProcessingLoop, this);
    }
}

void CNCHostCommunicator::StopProcessingThread() {
    keep_running = false;
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

uint32_t CNCHostCommunicator::GetNextSegmentID() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    return current_segment_id;
}

void CNCHostCommunicator::AddSegment(SPI_Poly_Segment_t seg) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    seg.segment_id = current_segment_id++;
    seg.crc16 = CalculateCRC16(reinterpret_cast<uint8_t*>(&seg), sizeof(SPI_Poly_Segment_t) - 2);
    transmit_queue.push_back(seg); 
}

void CNCHostCommunicator::SendPause() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    is_paused = true;
    
    SPI_Poly_Segment_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.flags = CNC_CMD_PAUSE;
    msg.crc16 = CalculateCRC16(reinterpret_cast<uint8_t*>(&msg), sizeof(msg) - 2);
    transmit_queue.push_front(msg);
}

void CNCHostCommunicator::SendResume() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    is_paused = false;
    
    SPI_Poly_Segment_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.flags = CNC_CMD_RESUME;
    msg.crc16 = CalculateCRC16(reinterpret_cast<uint8_t*>(&msg), sizeof(msg) - 2);
    transmit_queue.push_front(msg);
}

void CNCHostCommunicator::SendEmergencyStop() {
    SPI_Poly_Segment_t estop_msg;
    std::memset(&estop_msg, 0, sizeof(SPI_Poly_Segment_t));
    estop_msg.flags = CNC_CMD_IMMEDIATE_STOP;
    estop_msg.crc16 = CalculateCRC16(reinterpret_cast<uint8_t*>(&estop_msg), sizeof(estop_msg) - 2);
    
    std::lock_guard<std::mutex> lock(queue_mutex);
    transmit_queue.clear();             
    transmit_queue.push_front(estop_msg); 
}

void CNCHostCommunicator::SendSystemCommand(uint8_t cmd_flag) {
    std::lock_guard<std::mutex> lock(queue_mutex);
    transmit_queue.clear();
    is_paused = false; 

    SPI_Poly_Segment_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.flags = cmd_flag;
    msg.crc16 = CalculateCRC16(reinterpret_cast<uint8_t*>(&msg), sizeof(msg) - 2);
    transmit_queue.push_back(msg);
}

void CNCHostCommunicator::ClearQueue() {
    std::lock_guard<std::mutex> lock(queue_mutex);
    transmit_queue.clear();
    is_paused = false;

    SPI_Poly_Segment_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.flags = CNC_CMD_ABORT;
    msg.crc16 = CalculateCRC16(reinterpret_cast<uint8_t*>(&msg), sizeof(msg) - 2);
    transmit_queue.push_back(msg);
}

SPI_Status_Response_t CNCHostCommunicator::GetLastStatus() {
    std::lock_guard<std::mutex> lock(status_mutex);
    return last_response;
}

uint16_t CNCHostCommunicator::CalculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0x1021;
            else crc >>= 1;
        }
    }
    return crc;
}

void CNCHostCommunicator::ProcessingLoop() {
    //currently only simulated answer
    static uint8_t simulated_state = STATE_IDLE; 

    while (keep_running) {
        SPI_Poly_Segment_t tx_packet;
        std::memset(&tx_packet, 0, sizeof(SPI_Poly_Segment_t));
        bool is_real_segment = false;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (!transmit_queue.empty()) {
                uint8_t next_cmd = transmit_queue.front().flags & 0x0F;
                if (!is_paused || next_cmd != CNC_CMD_SEGMENT) {
                    tx_packet = transmit_queue.front();
                    transmit_queue.pop_front();
                    is_real_segment = true;
                }
            }
        }

        if (!is_real_segment) {
            tx_packet.flags = 0xFF;
        }

        SPI_Status_Response_t response;
        std::memset(&response, 0, sizeof(SPI_Status_Response_t));

        static int homing_timer = 0;
        static int probe_timer = 0;

        uint8_t cmd = tx_packet.flags & 0x0F;
        
        if (tx_packet.flags == 0xFF) {
            if (simulated_state == STATE_RUNNING) simulated_state = STATE_IDLE;
        } else {
            if (cmd == CNC_CMD_IMMEDIATE_STOP) {
                simulated_state = STATE_ESTOP;
                std::lock_guard<std::mutex> lock(queue_mutex);
                transmit_queue.clear(); 
            } 
            else if (cmd == CNC_CMD_ABORT) {
                if (simulated_state != STATE_ESTOP) simulated_state = STATE_IDLE;
            }
            else if (cmd == CNC_CMD_START_HOMING) {
                simulated_state = STATE_HOMING;
                homing_timer = 50;
            }
            else if (cmd == CNC_CMD_PAUSE) simulated_state = STATE_HOLD;
            else if (cmd == CNC_CMD_RESUME) {
                if (simulated_state == STATE_HOLD) simulated_state = STATE_RUNNING;
            } 
            else if (cmd == CNC_CMD_PROBE) {
                if (simulated_state == STATE_IDLE) simulated_state = STATE_RUNNING;
                
                if (++probe_timer > 25) {
                    std::lock_guard<std::mutex> lock(queue_mutex);
                    transmit_queue.clear();
                    probe_timer = 0;
                }
            }
            else if (cmd == CNC_CMD_SEGMENT) {
                if (simulated_state == STATE_IDLE) simulated_state = STATE_RUNNING;
                probe_timer = 0;
            }
        }
        if (simulated_state == STATE_HOMING) {
            if (--homing_timer <= 0) {
                simulated_state = STATE_IDLE;
            }
        }
        if (simulated_state != STATE_ESTOP) {
            response.last_processed_id = tx_packet.segment_id;
        }

        response.status = HOST_STATUS_ACK | (simulated_state << 4);
        response.free_segments = 32;
        {
            std::lock_guard<std::mutex> lock(status_mutex);
            last_response = response;
        }

        usleep(20000);
    }
}