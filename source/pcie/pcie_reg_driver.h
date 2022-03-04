#ifndef PROTOCOL_MEM_CFG_H
#define PROTOCOL_MEM_CFG_H

#include "stdint.h"

#define PROTOCL_SOF 0x55aa55aa

#define PCIE_POTOCOL_MEM_ADDR (0x70000000)
#define PCIE_ACK_MSG_OFFSET     2048
#define PCIE_POTOCOL_MEM_SIZE (4096u)


#define PCIE_IMAGE_MEM_ADDR (PCIE_POTOCOL_MEM_ADDR + PCIE_POTOCOL_MEM_SIZE)
#define PCIE_IMAGE_MEM_SZIE (0x0FF00000 - PCIE_POTOCOL_MEM_SIZE)

//read only
#define VDMA0_FRM_PTR_OFFSET (0)
#define VDMA1_FRM_PTR_OFFSET (1)
#define VDMA2_FRM_PTR_OFFSET (2)
#define VDMA3_FRM_PTR_OFFSET (3)
#define VDMA4_FRM_PTR_OFFSET (4)
#define VDMA5_FRM_PTR_OFFSET (5)
#define VDMA6_FRM_PTR_OFFSET (6)
#define VDMA7_FRM_PTR_OFFSET (7)
//read write
#define HOST2SOC_INFO1_OFFSET (8)
#define HOST2SOC_INFO2_OFFSET (9)
#define HOST2SOC_INFO3_OFFSET (10)
//read write
#define RAISE_SLV_IRQ_OFFSET (11)
//read only
#define SOC2HOST_INFO1_OFFSET (16)
#define SOC2HOST_INFO2_OFFSET (17)
#define SOC2HOST_INFO3_OFFSET (18)
#define SOC2HOST_INTR_CTL_OFFSET (19)
//read only
#define CLEAR_FROM_SLV_IRQ_OFFSET (0x70)  //read this reg, irq will clear

#define IMAGE_RING_BUF_NUM 7
#define IMAGE_BUFF_SIZE (1920*1090*2)
#define IMAGE_VSIZE_APPEND_SIZE 40

typedef struct {
    uint32_t sof;
    uint32_t cmd_index;
    uint32_t cmd_id;
    uint32_t cmd_sub_id;
    uint64_t* msg;
    uint8_t append_info[16];
    uint8_t channel;
    uint8_t isack;
    uint8_t crc;
    uint8_t rsvd;
} pcie_msg_t __attribute__ ((aligned(1)));

typedef struct {
    uint32_t sof;
    uint32_t ack_cmd_index;
    uint32_t ack_cmd_id;
    uint32_t ack_sub_cmd_id;
    uint64_t* ack_msg_buffer_addr;
    uint8_t  append_info[16];
    uint8_t  crc;
    uint8_t  channel;
    uint8_t  rsvd[2];
} pcie_ack_msg_t __attribute__ ((aligned(1)));

int pcie_reg_get_frm_ptr(uint32_t *base, uint8_t dev_id);
//slv rasie irq 2 host
int pcie_reg_raise_irq2slv(uint32_t *base);
//slv clear irq from host
int pcie_reg_clear_irq_from_slv(uint32_t *base);
//slv write info 2 host
int pcie_reg_write_host_info(uint32_t *base, uint32_t info[3]);
//slv read info from host
int pcie_reg_read_slv_info(uint32_t *base, uint32_t info[3]);

#endif // PROTOCOL_MEM_CFG_H
