#ifndef __DEV_SDK_H__
#define __DEV_SDK_H__

#include "stdint.h"
#include "../error.h"

#define PROTOCL_MSG_SOF 0x55aa55aa
#define PROTOCL_ACK_MSG_SOF 0xaa55aa55

#define ALL_CHANNEL 0xff


enum {
	SONY_ISX021,
	SONY_ISX019 = 1,
	SONY_ISX031,
	SONY_IMX390,
	SONY_IMX424,
	SONY_IMX490,
	SONY_IMX728,

	OV_OX1F10 = 10,
	OV_OX3C,
	OV_OX8B,
	OV_OX5B1S,

	MAX_SENSOR_NUM,
};

struct host_iic_ctl_t {
    uint8_t  addr;
    uint16_t reg;
    uint16_t fmt;
};



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


	CMD_HOST_UPDATE_QFLASH_FW = 0x10000100,

	CMD_SLV_HEART_BEAT = 0x00,
	CMD_SLV_SYNC_SDK_INFO = 0x01,

	CMD_SLV_CMD_GET_ONE_FRAME = 0x011,

	CMD_SLV_UPLOAD_MIPI_INFO = 0x021,
};

typedef struct {
	uint8_t addr;
	uint16_t reg;
	uint16_t data;
	uint16_t fmt;
} host_iic_ctl_data_t __attribute__ ((aligned(1)));

typedef struct {
	uint32_t sof;
	uint32_t cmd_index;
	uint32_t cmd_id;
	uint32_t sub_cmd_id;
	uint8_t  data[64];
	uint8_t  data_type; //data is addr info or direct info
	uint32_t  data_size; //
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
	uint32_t err_code;
	uint8_t  data[64];
	uint8_t  data_type; //data is addr info or direct info
	uint32_t  data_size;
	uint8_t  channel;   //0xff all channel
	uint8_t  msg_type;  //is ack
	uint8_t  crc;
	uint8_t  rsvd;
} pcie_ack_msg_t __attribute__ ((aligned(1)));

typedef void(*sdk_handler)(const pcie_msg_t* msg, pcie_ack_msg_t* ack_msg);

void pcie_sdk_init();
void pcie_sdk_hanlder();

#endif
