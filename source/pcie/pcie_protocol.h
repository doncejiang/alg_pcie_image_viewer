#ifndef PCIE_PROTOCOL_H
#define PCIE_PROTOCOL_H

#include "stdint.h"

typedef struct {
    uint32_t sof;
    uint32_t cmd_index;
    uint32_t cmd_id;
    uint32_t cmd_sub_id;
    uint64_t* msg;
    uint16_t append_info[16];
    uint8_t channel;
    uint8_t isack;
    uint8_t crc;
    uint8_t rsvd;
} pcie_msg_t;


#endif // PCIE_PROTOCOL_H
