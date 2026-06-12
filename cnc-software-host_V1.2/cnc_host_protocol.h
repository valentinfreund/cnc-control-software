#ifndef CNC_HOST_PROTOCOL_H
#define CNC_HOST_PROTOCOL_H

#include <cstdint>

// command flags
#define CNC_CMD_SEGMENT          0x00
#define CNC_CMD_IMMEDIATE_STOP   0x01
#define CNC_CMD_PAUSE            0x02
#define CNC_CMD_RESUME           0x03
#define CNC_CMD_START_HOMING     0x04
#define CNC_CMD_ABORT            0x05
#define CNC_CMD_PROBE            0x06

// feedback from stm32
#define HOST_STATUS_ACK          0x01
#define HOST_STATUS_NACK         0x02

enum CNC_State_t {
    STATE_INIT = 0,
    STATE_IDLE,
    STATE_HOMING,
    STATE_RUNNING,
    STATE_HOLD,
    STATE_ESTOP
};

// jerk segment
struct __attribute__((packed)) SPI_Poly_Segment_t {
    uint32_t segment_id;
    uint32_t total_ticks;

    float x_v0;  float x_a0;  float x_j;
    float z_v0;  float z_a0;  float z_j;
    float y1_v0; float y1_a0; float y1_j;
    float y2_v0; float y2_a0; float y2_j;

    uint8_t  directions;
    uint8_t  flags;
    uint16_t crc16;
};

// status response from stm32
struct __attribute__((packed)) SPI_Status_Response_t {
    uint32_t last_processed_id;
    uint8_t  status;            // Bits 0-3: ACK/NACK | Bits 4-7: CNC_State_t
    uint8_t  free_segments;
    uint16_t crc16;
};

#endif CNC_HOST_PROTOCOL_H