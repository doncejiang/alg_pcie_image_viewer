#ifndef __DEV_SDK_H__
#define __DEV_SDK_H__

#include "stdint.h"

#define PROTOCL_MSG_SOF 0x55aa55aa
#define PROTOCL_ACK_MSG_SOF 0xaa55aa55

#define ALL_CHANNEL 0xff

enum pcie_msg_type {
	PCIE_CMD_MSG_E,
	PCIE_ACK_MSG_E,
};

enum pcie_msg_data_type {
	DIRECT_INFO = 0,
	ADDR32_AND_SIZE32_INFO = 1,
	ADDR64_AND_SIZE32_INFO = 2,
};

enum host_cmd_id {
	CMD_HOST_HEART_BEAT = 0x10000000,
	CMD_HOST_SYNC_UTC_CLOCK = 0x10000001,

	CMD_HOST_START_CAMERA_STREAM = 0x10000011,
	CMD_HOST_DOWNLOAD_CAMERA_CFG = 0x10000012,
	CMD_HOST_CLOSE_CAMERA_STREAM = 0x10000013,

	CMD_HOST_GET_IIC_REG_DATA = 0x10000021,
	CMD_HOST_SET_IIC_REG_DATA = 0x10000022,

	CMD_HOST_GET_MIPI_POC_INFO = 0x10000031,

	CMD_SLV_HEART_BEAT = 0x00,
	CMD_SLV_SYNC_SDK_INFO = 0x01,

	CMD_SLV_CMD_GET_ONE_FRAME = 0x011,

	CMD_SLV_UPLOAD_MIPI_INFO = 0x021,
};

typedef struct {
	uint32_t sof;
	uint32_t cmd_index;
	uint32_t cmd_id;
	uint32_t sub_cmd_id;
	uint8_t  data[64];
	uint8_t  data_type; //data is addr info or direct info
	uint8_t  data_size; //
	uint8_t  channel;   //0xff all channel
	uint8_t  msg_type;  //is ack
	uint8_t  crc;
	uint8_t  rsvd;
} pcie_msg_t __attribute__ ((aligned(1)));

typedef struct {
	uint32_t sof;
	uint32_t ack_cmd_index;
	uint32_t ack_cmd_id;
	uint32_t ack_sub_cmd_id;
	uint8_t  data[64];
	uint8_t  data_type; //data is addr info or direct info
	uint8_t  data_size;
	uint8_t  channel;   //0xff all channel
	uint8_t  msg_type;  //is ack
	uint8_t  crc;
	uint8_t  rsvd;
} pcie_ack_msg_t __attribute__ ((aligned(1)));

void pcie_sdk_init();
void pcie_sdk_send_capture_one_frame(int channel, int frame_index);
void pcie_sdk_send_channel_frame_dt(uint8_t ch_dt[8], uint8_t buffer_id[8]);


void pcie_sdk_send_ack_msg(pcie_ack_msg_t *ack_msg);


pcie_msg_t* pcie_get_host_cmd();
void pcie_clear_host_cmd();

#endif
