#ifndef PCIE_PROTOCOL_H
#define PCIE_PROTOCOL_H

#include "stdint.h"
#include "dma_utils.h"
#include "pcie_reg_driver.h"

#define VDMA_NUM 8
#define VDMA_RING_FRM_NUM 7

class pcie_dev {
public:
    pcie_dev(int dev_id = 0);
    ~pcie_dev();
    int open_dev();
    int close_dev();
    int stream_on(void* cfg, uint8_t channel);
    int stream_off(uint8_t channel);
    int deque_image(char* image, uint32_t size, uint8_t channel);
    int get_decode_info(char* buffer, size_t size);
private:
    size_t read(char* buffer, size_t size, size_t off);
    size_t write(char* buffer, size_t size, size_t off);
private:
    int dev_id_ = -1;
    pcie_transfer_t trans;
    char c2h_dev_name_[64];
    char h2c_dev_name_[64];
    char reg_dev_name_[64];
    bool dev_is_open_ = false;
    uint32_t addr_table_[VDMA_NUM][VDMA_RING_FRM_NUM] = {0};
private:
    int get_frm_ptr(uint8_t dev_id);
    int raise_irq2slv();
    int clear_irq_from_slv();
    int write_host_info_reg(uint32_t info[3]);
    int read_slv_info_reg(uint32_t info[3]);
};


class pcie_manager {
private:
    pcie_manager();
public:
    static pcie_manager* get_instance();
    ~pcie_manager();
    int set_max_pcie_dev_number(uint8_t number);
    int open_dev();
    int close_dev();
    int stream_on(int channel);
    int stream_off(int channel);
    int deque_image(char* image, uint8_t ch);
    int enque_image(uint8_t ch);
    int i2c_read(uint8_t ii_addr, uint16_t reg, uint16_t& data, uint16_t fmt, uint8_t ch);
    int i2c_write(uint8_t ii_addr, uint16_t reg, uint16_t& data, uint16_t fmt, uint8_t ch);


private:
};

#endif // PCIE_PROTOCOL_H
