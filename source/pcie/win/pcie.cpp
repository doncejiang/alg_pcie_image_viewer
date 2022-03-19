#include "pcie.h"
#include <stdio.h>


#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include "stdlib.h"
#include "string.h"
#include <xdma_drv.h>


#define MAP_SIZE (32*1024UL)
#define MAP_MASK (MAP_SIZE - 1)
#define MEM_ALLOC_SIZE (3840 * 2166 * 2)
#define DEF_IMG_MEM_SIZE (3840 * 2160 * 2)




static int poll_irq_event(HANDLE device, int time_out = 33)
{
    int val = 0;
    xdma_read_device(device, 0, 4, (BYTE*)&val);
    return val;
}

pcie_dev::pcie_dev(int dev_id)
{
    this->dev_id_ = dev_id;

    //sprintf(c2h_dev_name_, C2H_DEVICE, dev_id);
    //sprintf(h2c_dev_name_, H2C_DEVICE, dev_id);
    //sprintf(reg_dev_name_, REG_DEVICE_NAME, dev_id);
    //sprintf(event_dev_name_, USER_EVENT_DEVICE_NAME, dev_id);
    //for (int i = 0; i < VDMA_NUM; ++i) {
    //    sprintf(img_event_dev_name_[i], IMG_EVENT_DEVICE_NAME, dev_id, i);
    //}

    for (int i = 0; i < VDMA_NUM; ++i) {
        for (int j = 0; j < VDMA_RING_FRM_NUM; ++j) {
            addr_table_[i][j] = i * VDMA_RING_FRM_NUM * DEF_IMG_MEM_SIZE + (DEF_IMG_MEM_SIZE * j) + PCIE_IMAGE_MEM_ADDR;
        }
    }
}

pcie_dev::~pcie_dev()
{
    if (dev_is_open_) {
        close_dev();
    }
}



int pcie_dev::wait_slv_cmd_ready_event()
{
    if (trans.cmd_event_device) {
        //auto ret = poll_irq_event(trans.cmd_event_fd);
        //if (ret >= 0) {
        //    pcie_reg_clear_irq_from_slv(trans.map_base_reg);
        //}
        //return ret;
    }
    return -1;
}

int pcie_dev::wait_image_ready_event(uint8_t channel)
{
    if (channel > (VDMA_NUM - 1) || !trans.img_event_device[channel]) return -1;
    return poll_irq_event(trans.img_event_device[channel]);
}

int pcie_dev::open_dev()
{
    printf("open dev%d start open\r\n", dev_id_);
    auto rc = xdma_open_device(&trans, 0);
    if (rc < 0) {
        printf("open dev%d failed\r\n", dev_id_);
        return -1;
    }


    dev_is_open_ = true;
    printf("open dev%d success\r\n", dev_id_);
    return 0;
}

int pcie_dev::close_dev()
{
    xdma_close_device(&trans, 0);
    return 0;
}

int pcie_dev::stream_on(void* cfg, uint8_t channel)
{
    pcie_msg_t msg;
    msg.cmd_id = 0x10000001;
    msg.channel = channel;
    //memcpy(trans.read_buffer, &msg, sizeof(msg));
    //
    //if (trans.h2c_fd) {
    //    int rc = write_from_buffer(h2c_dev_name_, trans.h2c_fd, trans.read_buffer, sizeof(msg), PCIE_POTOCOL_MEM_ADDR);
    //    if (rc < 0) {
    //        printf("stream on read to buffer failed size %d rc %d\n", sizeof(msg), rc);
    //        return -1;
    //    }
    //    printf("raise irq\r\n");
    //    raise_irq2slv();
    //    //if (trans.map_base_reg) {
    //        //trans.map_base_reg[11] = 1; //raise irq
    //    //}
    //}

    return 0;
}

int pcie_dev::stream_off(uint8_t channel)
{
    return 0;
}

int pcie_dev::get_decode_info(char* buffer, size_t size)
{
    pcie_msg_t msg;

    if (trans.c2h0_device) {
        auto rc = xdma_read_device(trans.c2h0_device, PCIE_POTOCOL_MEM_ADDR + 2048, sizeof(msg), (BYTE *)trans.read_dev_buffer);
                //read_to_buffer(c2h_dev_name_, trans.c2h_fd, trans.write_buffer, sizeof(msg), PCIE_POTOCOL_MEM_ADDR + 2048);
        memcpy(&msg, trans.read_dev_buffer, sizeof(msg));
        return rc;
    }
    return -1;
}
// 0   1   2   3  4  5
//3    2   6   7  5  4
//12   13  15  14 10 11
int pcie_dev::deque_image(char* image, uint32_t size, uint8_t channel)
{
    if (channel > VDMA_NUM || !image) return -1;
    static int buff[8];
    int grey_code = 0;
    //static uint8_t s_frm7_grey2dec_lut[16] = {0xff, 0, 2, 1,  6, 5, 3, 4, 0xff, 6,  4, 5,  0,  1,  3,  2};
    static uint8_t s_frm6_grey2dec_lut[16] = {0xff, 0xff, 1, 0,  5, 4, 2, 3, 0xff, 0xff,  4, 5,  0,  1,  3,  2};
    grey_code = get_frm_ptr(channel);
    if (grey_code > 15) grey_code = 15;
    if (grey_code == buff[channel]) {
        return -1;
    } else {
        buff[channel] = grey_code;
    }
    auto ptr = s_frm6_grey2dec_lut[grey_code];
    if (ptr > (VDMA_RING_FRM_NUM - 1)) ptr = (VDMA_RING_FRM_NUM - 1);

    if (ptr == 0) {
        ptr = (VDMA_RING_FRM_NUM - 1);
    } else {
        ptr -= 1;
    }

    return read(image, size, addr_table_[channel][ptr]);
}

size_t pcie_dev::read(char* buffer, size_t size, size_t off)
{
    uint64_t addr = off;
    //std::lock_guard<std::mutex> lck(mutex_);
    mutex_.lock();
    if (trans.c2h0_device) {
        auto rc = xdma_read_device(trans.c2h0_device, addr, size, (BYTE*)trans.read_img_buffer[0]);
                                   //read_to_buffer(c2h_dev_name_, trans.c2h_fd, trans.write_buffer, size, addr);
        if (rc < 0) {
            printf("dev %d read to buffer failed size %d rc %d\n", dev_id_, size, rc);
            return -1;
            mutex_.unlock();
        }
        memcpy(buffer, trans.read_img_buffer[0], size);
    }
    mutex_.unlock();
    return 0;
}

size_t pcie_dev::write(char* buffer, size_t size, size_t off)
{
    //std::lock_guard<std::mutex> lck(mutex_);
    mutex_.lock();
    memcpy(trans.write_dev_buffer, buffer, size);
    if (trans.h2c0_device) {
        int rc = xdma_write_device(trans.h2c0_device, off, size, (BYTE*)trans.write_dev_buffer);
        if (rc < 0) {
            mutex_.unlock();
            printf("error index %d, read to buffer failed size %d rc %d\n", 1, size, rc);
            return -1;
        }
    }
    mutex_.unlock();
    return 0;
}

int pcie_dev::get_channel_decode_info(hw_sts& sts)
{
    pcie_msg_t msg;
    if (get_pcie_msg(msg)) {

        return 0;
    }
    return -1;
}

int pcie_dev::get_pcie_msg(pcie_msg_t &msg)
{
    //if (trans.c2h_fd) {
    //    auto rc = read_to_buffer(c2h_dev_name_, trans.c2h_fd, trans.write_buffer, sizeof(msg), PCIE_POTOCOL_MEM_ADDR + PCIE_ACK_MSG_OFFSET);
    //    memcpy(&msg, trans.write_buffer, sizeof(msg));
    //    if (rc >= 0) {
    //        return rc;
    //    }
    //}
    return -1;
}

int pcie_dev::get_frm_ptr(uint8_t dev_id)
{
    if (trans.user_device) {
        return pcie_reg_get_frm_ptr(&trans, dev_id);
    }
    return -1;
}

int pcie_dev::raise_irq2slv()
{
    if (trans.user_device) {
        return pcie_reg_raise_irq2slv(&trans);
    }
    return -1;
}

int pcie_dev::clear_irq_from_slv()
{
    if (trans.user_device) {
        return pcie_reg_clear_irq_from_slv(&trans);
    }
    return -1;
}

int pcie_dev::write_host_info_reg(uint32_t info[3])
{
    //if (trans.map_base_reg) {
    //    return pcie_reg_write_host_info(trans.map_base_reg, info);
    //}
    return -1;
}

int pcie_dev::read_slv_info_reg(uint32_t info[3])
{
    //if (trans.map_base_reg) {
    //    return pcie_reg_read_slv_info(trans.map_base_reg, info);
    //}
    return -1;
}
