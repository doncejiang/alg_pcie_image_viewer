#include "protocol_sdk.h"
#include "mem_partion.h"
#include "../../driver/pcie_reg_space/pcie_reg_space.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "crc.h"
#include "../update/update.h"

static pcie_msg_t* g_pcie_host_msg_ptr = (pcie_msg_t *)PCIE_POTOCOL_HOST_CMD_MEM_ADDR;
static pcie_ack_msg_t* g_pcie_host_ack_msg_ptr = (pcie_ack_msg_t *)(PCIE_POTOCOL_HOST_CMD_ACK_MEM_ADDR);

static pcie_msg_t* g_pcie_slv_msg_ptr = (pcie_msg_t *)PCIE_POTOCOL_SLV_CMD_MEM_ADDR;
static pcie_ack_msg_t* g_pcie_slv_ack_msg_ptr = (pcie_ack_msg_t *)(PCIE_POTOCOL_SLV_CMD_ACK_MEM_ADDR);

static SemaphoreHandle_t s_pcie_mutex = NULL;
static uint32_t g_cmd_index = 0;


static inline void raise_irq2host()
{
	if (!s_pcie_mutex) return;
	xSemaphoreTake(s_pcie_mutex, portMAX_DELAY);
	pcie_reg_slv_raise_irq2host();
	xSemaphoreGive(s_pcie_mutex);
}

static inline void clear_irq2host()
{
	if (!s_pcie_mutex) return;
	xSemaphoreTake(s_pcie_mutex, portMAX_DELAY);
	pcie_reg_slv_clear_irq2host();
	xSemaphoreGive(s_pcie_mutex);
}

static void pcie_sdk_send_ack_msg(pcie_ack_msg_t *ack_msg)
{
	memcpy(g_pcie_host_ack_msg_ptr, ack_msg, sizeof(pcie_ack_msg_t));
	pcie_reg_slv_raise_irq2host();
}

static void pcie_sdk_send_msg(pcie_msg_t *msg)
{
	memcpy(g_pcie_slv_msg_ptr, msg, sizeof(pcie_msg_t));
	pcie_reg_slv_raise_second_irq2host();
}

static const pcie_msg_t* pcie_sdk_get_host_cmd()
{
	return g_pcie_host_msg_ptr;
}

static void pcie_sdk_clear_host_cmd()
{
	memset(g_pcie_host_msg_ptr, 0, sizeof(pcie_msg_t));
}

/////////////////////////////
static void pcie_sdk_iic_read(const pcie_msg_t* msg, pcie_ack_msg_t* ack_msg)
{
	int ret = 0;
	uint8_t cmd_channel = msg->channel;
	if (cmd_channel == 0xff) return;
	host_iic_ctl_data_t* iic_ctl = (host_iic_ctl_data_t *)msg->data;
	uint8_t deser_id = cmd_channel / 2;
	uint8_t port = cmd_channel % 2;
	uint16_t data_16 = 0;
	deser_change_port(deser_id, port);
	xil_printf("pcie sdk iic read addr %x, reg %x fmt %x\r\n", iic_ctl->addr, iic_ctl->reg, iic_ctl->fmt);
	switch (iic_ctl->fmt) {
	case 0x0808:
		ret = iic_reg8_read(deseres_get_iic_id(deser_id), iic_ctl->addr, iic_ctl->reg, &ack_msg->data[0]);
		break;
	case 0x1608:
		ret = iic_reg16_read(deseres_get_iic_id(deser_id), iic_ctl->addr, iic_ctl->reg, &ack_msg->data[0]);
		break;
	case 0x0816:
		ret = iic_reg8_read16(deseres_get_iic_id(deser_id), iic_ctl->addr, iic_ctl->reg, &data_16);
		ack_msg->data[0] = data_16 & 0xff;
		ack_msg->data[1] = (data_16 & 0xff00) >> 8;
		break;
	}

	if (ret <= 0) {
		ack_msg->err_code = 0xe00;
	} else {
		ack_msg->err_code = 0;
	}
	ack_msg->msg_type = PCIE_ACK_MSG_E;
}

static void pcie_sdk_iic_write(const pcie_msg_t* msg, pcie_ack_msg_t* ack_msg)
{
	int ret = 0;
	uint8_t cmd_channel = msg->channel;
	if (cmd_channel == 0xff) return;
	host_iic_ctl_data_t* iic_ctl = (host_iic_ctl_data_t *)msg->data;
	uint8_t deser_id = cmd_channel / 2;
	uint8_t port = cmd_channel % 2;
	deser_change_port(deser_id, port);
	xil_printf("pcie sdk iic write addr %x, reg %x fmt %x data %x\r\n", iic_ctl->addr, iic_ctl->reg, iic_ctl->fmt, iic_ctl->data);
	switch (iic_ctl->fmt) {
	case 0x0808:
		ret = iic_reg8_write(deseres_get_iic_id(deser_id), iic_ctl->addr, iic_ctl->reg, iic_ctl->data);
		break;
	case 0x1608:
		ret = iic_reg16_write(deseres_get_iic_id(deser_id), iic_ctl->addr, iic_ctl->reg, iic_ctl->data);
		break;
	case 0x0816:
		ret = iic_reg8_write16(deseres_get_iic_id(deser_id), iic_ctl->addr, iic_ctl->reg, iic_ctl->data);
		break;
	}
	if (ret <= 0) {
		ack_msg->err_code = 0xe00;
	}
	ack_msg->msg_type = PCIE_ACK_MSG_E;
}

static void pcie_sdk_start_stream(const pcie_msg_t* msg, pcie_ack_msg_t* ack_msg)
{
	uint8_t type[4];
	memcpy(type, msg->data, 4);
	app_init_sensors_by_sensor_type(type);
	ack_msg->msg_type = PCIE_ACK_MSG_E;
}

static void pcie_sdk_get_hard_sts(const pcie_msg_t* msg, pcie_ack_msg_t* ack_msg)
{
	float vol[8], cur[8];
	uint8_t dt[8];
	app_get_cur_hardware_sts(vol, cur, dt);
	uint16_t tmp[8];
	for (int i = 0; i < 8; ++i) {
		tmp[i] = (uint16_t)(vol[i] * 100);
	}
	memcpy(&ack_msg->data[0], tmp, 16);
	for (int i = 0; i < 8; ++i) {
		tmp[i] = (uint16_t)(cur[i] * 100);
	}
	memcpy(&ack_msg->data[16], tmp, 16);
	memcpy(&ack_msg->data[32], dt, 8);
	ack_msg->msg_type = PCIE_ACK_MSG_E;
}

#define MAX_SDK_NUM 256

struct {
	uint32_t cmd_id;
	sdk_handler handler;
} pcie_sdk_cmd_desc[MAX_SDK_NUM];
static uint8_t s_sdk_cmd_num = 0;


static void pcie_sdk_register(uint32_t cmd_id, sdk_handler handler)
{
	if (s_sdk_cmd_num >= MAX_SDK_NUM) return;

	pcie_sdk_cmd_desc[s_sdk_cmd_num].cmd_id = cmd_id;
	pcie_sdk_cmd_desc[s_sdk_cmd_num].handler = handler;
	xil_printf("pcie sdk register cmd %x\r\n", cmd_id);

	++s_sdk_cmd_num;
}

static void pcie_sdk_update_qflash_fw(const pcie_msg_t* msg, pcie_ack_msg_t* ack_msg)
{
	if (msg->data_type == DIRECT_INFO) {
		xil_printf("[cmd] update qflash datatype is not correct\r\n");
		ack_msg->err_code = ALG_PCIE_COM_DATA_TYPE_ERROR;
		ack_msg->msg_type = PCIE_ACK_MSG_E;
		return;
	}

	uint64_t addr = 0;
	if (msg->data_type == ADDR32_AND_SIZE32_INFO) {
		for (int i = 0; i < 4; ++i) {
			addr |= (msg->data[i] << (8 * i));
		}
	}

	if (addr != PCIE_QFLASH_BIN_MEM_ADDR) {
		xil_printf("[cmd] update qflash addr is not correct\r\n");
		ack_msg->err_code = ALG_PCIE_COM_DATA_LEN_ERROR;
		ack_msg->msg_type = PCIE_ACK_MSG_E;
		return;
	}

	unsigned char crc = 0;
	crc = cal_crc8((char *)addr, msg->data_size);

	if (crc != msg->crc) {
		xil_printf("[cmd] update qflash addr is not correct\r\n");
		ack_msg->err_code = ALG_PCIE_COM_DATA_CRC_ERROR;
		ack_msg->msg_type = PCIE_ACK_MSG_E;
		return;
	}


	update_fw((char *)addr, msg->data_size);

}


void pcie_sdk_init()
{
	s_pcie_mutex = xSemaphoreCreateMutex();

	clear_irq2host();

	pcie_sdk_register(CMD_HOST_START_CAMERA_STREAM, pcie_sdk_start_stream);
	pcie_sdk_register(CMD_HOST_GET_MIPI_POC_INFO, pcie_sdk_get_hard_sts);
	pcie_sdk_register(CMD_HOST_GET_IIC_REG_DATA, pcie_sdk_iic_read);
	pcie_sdk_register(CMD_HOST_SET_IIC_REG_DATA, pcie_sdk_iic_write);
	pcie_sdk_register(CMD_HOST_UPDATE_QFLASH_FW, pcie_sdk_update_qflash_fw);
}

void pcie_sdk_hanlder()
{
	pcie_ack_msg_t ack_msg;
	uint32_t cmd_id = pcie_sdk_get_host_cmd()->cmd_id;
	for (int i = 0; i < s_sdk_cmd_num; ++i) {
		if (pcie_sdk_cmd_desc[i].cmd_id == cmd_id && pcie_sdk_cmd_desc[i].handler) {
			pcie_sdk_cmd_desc[i].handler(pcie_sdk_get_host_cmd(), &ack_msg);
			pcie_sdk_send_ack_msg(&ack_msg);
		}
	}
}
