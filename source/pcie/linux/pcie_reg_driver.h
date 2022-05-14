#ifndef PROTOCOL_MEM_CFG_H
#define PROTOCOL_MEM_CFG_H

#include "stdint.h"
#include "../mem_partion.h"

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
#define CLEAR_FROM_SLV2_IRQ_OFFSET (0x74)  //read this reg, irq will clear

#define IMAGE_RING_BUF_NUM 7


int pcie_reg_get_frm_ptr(uint32_t *base, uint8_t dev_id);
//slv rasie irq 2 host
int pcie_reg_raise_irq2slv(uint32_t *base);
//slv clear irq from host
int pcie_reg_clear_irq2_from_slv(uint32_t *base);

int pcie_reg_clear_irq_from_slv(uint32_t *base);
//slv write info 2 host
int pcie_reg_write_host_info(uint32_t *base, uint32_t info[3]);
//slv read info from host
int pcie_reg_read_slv_info(uint32_t *base, uint32_t info[3]);

#endif // PROTOCOL_MEM_CFG_H
