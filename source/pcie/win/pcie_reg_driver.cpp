#include "pcie_reg_driver.h"
#include "xdma_drv.h"

int pcie_reg_get_frm_ptr(struct pcie_trans *trans, uint8_t dev_id)
{
    if (!trans->user_device || dev_id > 7) return -1;
    uint32_t val;
    xdma_read_device(trans->user_device, dev_id * 4, 4, (BYTE *)&val);
    return val;
}
//slv rasie irq 2 host
int pcie_reg_raise_irq2slv(struct pcie_trans *trans)
{
    if (!trans->user_device) return -1;
    uint32_t val = 1;
    xdma_write_device(trans->user_device,  RAISE_SLV_IRQ_OFFSET * 4, 4, (BYTE *)&val);
    return 0;
}
//slv clear irq from host
int pcie_reg_clear_irq_from_slv(struct pcie_trans *trans)
{
    //uint32_t data = *(base + (CLEAR_FROM_SLV_IRQ_OFFSET / 4));
    if (!trans->user_device) return -1;
    uint32_t val;
    xdma_read_device(trans->user_device, RAISE_SLV_IRQ_OFFSET * 4, 4, (BYTE *)&val);
    return 0;
}
//slv write info 2 host
int pcie_reg_write_host_info(uint32_t *base, uint32_t info[3])
{
    *(base + HOST2SOC_INFO1_OFFSET) = info[0];
    *(base + HOST2SOC_INFO2_OFFSET) = info[1];
    *(base + HOST2SOC_INFO3_OFFSET) = info[2];
    return 0;
}
//slv read info from host
int pcie_reg_read_slv_info(uint32_t *base, uint32_t info[3])
{
    info[0] = *(base + SOC2HOST_INFO1_OFFSET);
    info[1] = *(base + SOC2HOST_INFO2_OFFSET);
    info[2] = *(base + SOC2HOST_INFO3_OFFSET);
    return 0;
}
