#include "pcie.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/mman.h>

#define MAP_SIZE (32*1024UL)
#define MAP_MASK (MAP_SIZE - 1)
#define MEM_ALLOC_SIZE (3840 * 2166 * 2)
#define DEF_IMG_MEM_SIZE (3840 * 2160 * 2)
#define CMD_MEM_ALLOC_SIZE (1024 * 1024) //1M


#define H2C_DEVICE "/dev/xdma%d_h2c_0"
#define C2H_DEVICE "/dev/xdma%d_c2h_0"
#define H2C_CMD_DEVICE "/dev/xdma%d_h2c_1"
#define C2H_CMD_DEVICE "/dev/xdma%d_c2h_1"
#define REG_DEVICE_NAME "/dev/xdma%d_user"
#define USER_EVENT_DEVICE_NAME "/dev/xdma%d_events_8"
#define IMG_EVENT_DEVICE_NAME "/dev/xdma%d_events_%d"




static int poll_irq_event(int fd, int time_out = 33)
{
    struct pollfd fds[] = {
          { fd, POLLIN }
      };

    int val;
    //val = poll(fds, 1, time_out);
    val = read(fd, &val, 4);
    return val;

}

pcie_dev::pcie_dev(int dev_id)
{
    this->dev_id_ = dev_id;

    sprintf(c2h_dev_name_, C2H_DEVICE, dev_id);
    sprintf(h2c_dev_name_, H2C_DEVICE, dev_id);
    sprintf(c2h_cmd_dev_name_, C2H_CMD_DEVICE, dev_id);
    sprintf(h2c_cmd_dev_name_, H2C_CMD_DEVICE, dev_id);
    sprintf(reg_dev_name_, REG_DEVICE_NAME, dev_id);
    sprintf(event_dev_name_, USER_EVENT_DEVICE_NAME, dev_id);
    for (int i = 0; i < VDMA_NUM; ++i) {
        sprintf(img_event_dev_name_[i], IMG_EVENT_DEVICE_NAME, dev_id, i);
    }

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



int pcie_dev::wait_slv_cmd_ready_event(int timeout)
{
    if (trans.cmd_event_fd > 0) {
        //pcie_reg_clear_irq_from_slv(trans.map_base_reg);
        auto ret = poll_irq_event(trans.cmd_event_fd, timeout);
        if (ret >= 0) {
            pcie_reg_clear_irq_from_slv(trans.map_base_reg);
        }
        return ret;
    }
    return -1;
}

int pcie_dev::wait_image_ready_event(uint8_t channel)
{
    if (channel > 7 || trans.img_event_fd[channel] < 0) return -1;
    return poll_irq_event(trans.img_event_fd[channel]);
}

int pcie_dev::open_dev()
{
    printf("open dev%d start open\r\n", dev_id_);
    trans.h2c_fd = open(h2c_dev_name_, O_RDWR);
    if (trans.h2c_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            h2c_dev_name_, trans.h2c_fd);
        perror("open device");
        return -1;
    }

    trans.c2h_fd = open(c2h_dev_name_, O_RDWR);
    if (trans.c2h_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            c2h_dev_name_, trans.c2h_fd);
        perror("open device");
        return - 1;
    }

    trans.h2c_cmd_fd = open(h2c_cmd_dev_name_, O_RDWR);
    if (trans.h2c_cmd_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            h2c_cmd_dev_name_, trans.h2c_cmd_fd);
        perror("open device");
        return -1;
    }

    trans.c2h_cmd_fd = open(c2h_cmd_dev_name_, O_RDWR);
    if (trans.c2h_cmd_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            c2h_cmd_dev_name_, trans.c2h_cmd_fd);
        perror("open device");
        return - 1;
    }

    trans.cmd_event_fd = open(event_dev_name_, O_RDONLY);
    if (trans.cmd_event_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
        event_dev_name_, trans.reg_fd);
        perror("open event device failed");
        return -1;
    }


    for (int i = 0; i < 8; ++i) {
        trans.img_event_fd[i] = open(img_event_dev_name_[i], O_RDONLY);
        if (trans.img_event_fd[i] < 0) {
            fprintf(stderr, "unable to open device %s, %d.\n",
            img_event_dev_name_[i], trans.reg_fd);
            perror("open event device failed");
            return -1;
        }
    }

    trans.reg_fd = open(reg_dev_name_, O_RDWR);
    if (trans.reg_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
        reg_dev_name_, trans.reg_fd);
        perror("open reg device failed");
        return -1;
    }

    /* map one page */
    trans.map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, trans.reg_fd, 0);
    if (trans.map_base == (void *)-1) {
        printf("error unmap\n");
        printf("Memory mapped at address %p.\n", trans.map_base);
        fflush(stdout);
    }

    trans.map_base_reg = (uint32_t*)trans.map_base;

    char *read_allocated = nullptr;
    char *write_allocated = nullptr;
    constexpr size_t alloc_size = MEM_ALLOC_SIZE;

    posix_memalign((void **)&read_allocated, 4096 , alloc_size + 4096);
    if (!read_allocated) {
        fprintf(stderr, "OOM %u.\n", alloc_size + 4096);
    }

    trans.read_img_buffer = read_allocated;
    posix_memalign((void **)&write_allocated, 4096 , alloc_size + 4096);
    if (!write_allocated) {
        fprintf(stderr, "OOM %u.\n", alloc_size + 4096);
    }

    trans.write_img_buffer = write_allocated;

    auto cmd_alloc_size = CMD_MEM_ALLOC_SIZE;
    posix_memalign((void **)&read_allocated, 4096 , alloc_size + 4096);
    if (!read_allocated) {
        fprintf(stderr, "OOM %u.\n", alloc_size + 4096);
    }

    trans.read_cmd_buffer = read_allocated;
    posix_memalign((void **)&write_allocated, 4096 , alloc_size + 4096);
    if (!write_allocated) {
        fprintf(stderr, "OOM %u.\n", alloc_size + 4096);
    }

    trans.write_cmd_buffer = write_allocated;

    dev_is_open_ = true;
    printf("open dev%d success\r\n", dev_id_);
    return 0;
}

int pcie_dev::close_dev()
{
    if (trans.h2c_fd) {
        close(trans.h2c_fd);
    }
    if (trans.h2c_fd) {
        close(trans.h2c_fd);
    }

    if (trans.cmd_event_fd) {
        close(trans.cmd_event_fd);
    }


    for (int i = 0; i < 8; ++i) {
        if (trans.img_event_fd[i]) {
            close(trans.img_event_fd[i]);
        }
    }

    if (trans.reg_fd) {
        if (munmap(trans.map_base, MAP_SIZE) == -1)
            printf("error unmap\n");
        close(trans.reg_fd);
    }

    if (trans.read_img_buffer ) {
        free(trans.read_img_buffer);
    }

    if (trans.write_img_buffer ) {
        free(trans.write_img_buffer);
    }

    dev_is_open_ = false;
}

int pcie_dev::stream_on(void* cfg, uint8_t channel)
{
    printf("stream on\r\n");
    pcie_msg_t msg;
    msg.cmd_id = CMD_HOST_START_CAMERA_STREAM;
    msg.channel = channel;
    clear_irq_from_slv();
    for (int i = 0; i < 4; ++i) {
        msg.data[i] = SONY_ISX021;
    }
    this->pcie_write(trans.h2c_cmd_fd, (char *)&msg, sizeof(msg), PCIE_POTOCOL_MEM_ADDR);
    raise_irq2slv();
    auto ret = wait_slv_cmd_ready_event(15000);
    if (ret < 0) return ret;

    return 0;
}

int pcie_dev::stream_off(uint8_t channel)
{
    return 0;
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
    //TODO:image ptr use align malloc to avoid memcpy
    img_wr_mutex_.lock();
    auto rc = pcie_read(trans.c2h_fd, image, size, addr_table_[channel][ptr]);
    img_wr_mutex_.unlock();
    return rc;
}

size_t pcie_dev::pcie_read(int c2h_fd, char* buffer, size_t size, size_t off, bool is_cmd)
{
    uint64_t addr = off;
    //std::lock_guard<std::mutex> lck(mutex_);
    char* write_buffer;
    if (!is_cmd)
        write_buffer = trans.write_img_buffer;
    else
        write_buffer = trans.write_cmd_buffer;
    if (trans.c2h_fd) {
        auto rc = read_to_buffer(c2h_dev_name_, c2h_fd, write_buffer, size, addr);
        if (rc < 0) {
            printf("dev %d read to buffer failed size %d rc %d\n", dev_id_, size, rc);
            return -1;
        }
        memcpy(buffer, write_buffer, size);
    }
    return 0;
}

size_t pcie_dev::pcie_write(int h2c_fd, char* buffer, size_t size, size_t off, bool is_cmd)
{

    //std::lock_guard<std::mutex> lck(mutex_);
    char* read_buffer;
    if (!is_cmd)
        read_buffer = trans.write_img_buffer;
    else
        read_buffer = trans.write_cmd_buffer;
    memcpy(read_buffer, buffer, size);
    if (trans.h2c_fd) {
        int rc = write_from_buffer(H2C_DEVICE, h2c_fd, read_buffer, size, off);
        if (rc < 0) {
            printf("error index %d, read to buffer failed size %d rc %d\n", 1, size, rc);
            return -1;
        }
    }
    return 0;
}

int pcie_dev::get_channel_decode_info(hw_sts& sts)
{
    cmd_wr_mutex_.lock();
    pcie_msg_t msg;
    msg.cmd_id = CMD_HOST_GET_MIPI_POC_INFO;
    msg.channel = 0xff;

    this->pcie_write(trans.h2c_cmd_fd, (char *)&msg, sizeof(msg), PCIE_POTOCOL_MEM_ADDR, true);
    raise_irq2slv();

    auto ret = wait_slv_cmd_ready_event(30);

    if (ret < 0) {
        cmd_wr_mutex_.unlock();
        return ret;
    }
    pcie_ack_msg_t ack_msg;
    this->pcie_read(trans.c2h_cmd_fd, (char *)&ack_msg, sizeof(ack_msg), PCIE_POTOCOL_HOST_CMD_ACK_MEM_ADDR, true);
    if (ack_msg.msg_type == PCIE_ACK_MSG_E) {
        for (int i = 0; i < 8; ++i) {
            sts.dt[i] = ack_msg.data[32 + i];
            sts.vol[i] = (float)((uint16_t)(ack_msg.data[2 * i] | ack_msg.data[2 * i + 1] << 8)) / 100.0;
            sts.cur[i] = (float)((uint16_t)(ack_msg.data[2 * i + 16] | ack_msg.data[2 * i + 1 + 16] << 8)) / 100.0;
        }
    }
    cmd_wr_mutex_.unlock();
    return 0;
}

int pcie_dev::i2c_read(uint8_t ch_id, uint8_t addr, uint16_t reg, uint16_t& data, uint16_t fmt)
{
    cmd_wr_mutex_.lock();
    pcie_msg_t msg;
    msg.cmd_id = CMD_HOST_GET_IIC_REG_DATA;
    msg.channel = ch_id;

    host_iic_ctl_t *iic_ctl = (host_iic_ctl_t *)msg.data;
    iic_ctl->addr = addr;
    iic_ctl->reg = reg;
    iic_ctl->fmt = fmt;
    this->pcie_write(trans.h2c_cmd_fd, (char *)&msg, sizeof(msg), PCIE_POTOCOL_MEM_ADDR, true);
    raise_irq2slv();

    auto ret = wait_slv_cmd_ready_event(10);

    if (ret < 0) {
        cmd_wr_mutex_.unlock();
        return ret;
    }
    pcie_ack_msg_t ack_msg;
    this->pcie_read(trans.c2h_cmd_fd, (char *)&ack_msg, sizeof(ack_msg), PCIE_POTOCOL_HOST_CMD_ACK_MEM_ADDR, true);
    if (ack_msg.msg_type == PCIE_ACK_MSG_E) {
        if (fmt == 0x1608 || fmt == 0x0808) {
            data = ack_msg.data[0];
        } else {
            data = ack_msg.data[0] | (ack_msg.data[1] << 8);
        }
    }
    printf("sdk iic read %x, error code %x\r\n", data, ack_msg.err_code);
    cmd_wr_mutex_.unlock();
    return 0;
}

int pcie_dev::get_pcie_msg(pcie_msg_t &msg)
{
    //if (trans.c2h_fd) {
    //    auto rc = read_to_buffer(c2h_dev_name_, trans.c2h_fd, trans.write_buffer, sizeof(msg), PCIE_POTOCOL_HOST_CMD_ACK_MEM_ADDR);
    //    memcpy(&msg, trans.write_buffer, sizeof(msg));
    //    if (rc >= 0) {
    //        return rc;
    //    }
    //}
    return -1;
}

int pcie_dev::get_frm_ptr(uint8_t dev_id)
{
    if (trans.map_base_reg) {
        return pcie_reg_get_frm_ptr(trans.map_base_reg, dev_id);
    }
    return -1;
}

int pcie_dev::raise_irq2slv()
{
    if (trans.map_base_reg) {
        return pcie_reg_raise_irq2slv(trans.map_base_reg);
    }
    return -1;
}

int pcie_dev::clear_irq_from_slv()
{
    if (trans.map_base_reg) {
        return pcie_reg_clear_irq_from_slv(trans.map_base_reg);
    }
    return -1;
}

int pcie_dev::write_host_info_reg(uint32_t info[3])
{
    if (trans.map_base_reg) {
        return pcie_reg_write_host_info(trans.map_base_reg, info);
    }
    return -1;
}

int pcie_dev::read_slv_info_reg(uint32_t info[3])
{
    if (trans.map_base_reg) {
        return pcie_reg_read_slv_info(trans.map_base_reg, info);
    }
    return -1;
}
