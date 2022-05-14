#include <pcie_reg_driver.h>


int pcie_reg_get_frm_ptr(uint32_t *base, uint8_t dev_id)
{
    if (!base || dev_id > 7) return -1;
    return *(base + (dev_id));
}
//slv rasie irq 2 host
int pcie_reg_raise_irq2slv(uint32_t *base)
{
    if (!base) return -1;
    *(base + RAISE_SLV_IRQ_OFFSET) = 1;
}
//slv clear irq from host
int pcie_reg_clear_irq_from_slv(uint32_t *base)
{
    uint32_t data = *(base + (CLEAR_FROM_SLV_IRQ_OFFSET / 4));
    return data;
}

int pcie_reg_clear_irq2_from_slv(uint32_t *base)
{
    uint32_t data = *(base + (CLEAR_FROM_SLV2_IRQ_OFFSET / 4));
    return data;
}
//slv write info 2 host
int pcie_reg_write_host_info(uint32_t *base, uint32_t info[3])
{
    *(base + HOST2SOC_INFO1_OFFSET) = info[0];
    *(base + HOST2SOC_INFO2_OFFSET) = info[1];
    *(base + HOST2SOC_INFO3_OFFSET) = info[2];
}
//slv read info from host
int pcie_reg_read_slv_info(uint32_t *base, uint32_t info[3])
{
    info[0] = *(base + SOC2HOST_INFO1_OFFSET);
    info[1] = *(base + SOC2HOST_INFO2_OFFSET);
    info[2] = *(base + SOC2HOST_INFO3_OFFSET);
}
