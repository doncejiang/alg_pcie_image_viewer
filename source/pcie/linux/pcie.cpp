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
#include "crc8.h"

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
    val = poll(fds, 1, time_out);
    //val = read(fd, &val, 4);
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
    trans.h2c_fd = open(h2c_dev_name_, O_RDWR | O_NONBLOCK);
    if (trans.h2c_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            h2c_dev_name_, trans.h2c_fd);
        perror("open device");
        return -1;
    }

    trans.c2h_fd = open(c2h_dev_name_, O_RDWR | O_NONBLOCK);
    if (trans.c2h_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            c2h_dev_name_, trans.c2h_fd);
        perror("open device");
        return - 1;
    }

    trans.h2c_cmd_fd = open(h2c_cmd_dev_name_, O_RDWR | O_NONBLOCK);
    if (trans.h2c_cmd_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            h2c_cmd_dev_name_, trans.h2c_cmd_fd);
        perror("open device");
        return -1;
    }

    trans.c2h_cmd_fd = open(c2h_cmd_dev_name_, O_RDWR | O_NONBLOCK);
    if (trans.c2h_cmd_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            c2h_cmd_dev_name_, trans.c2h_cmd_fd);
        perror("open device");
        return - 1;
    }

    trans.cmd_event_fd = open(event_dev_name_, O_RDONLY | O_NONBLOCK);
    if (trans.cmd_event_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
        event_dev_name_, trans.reg_fd);
        perror("open event device failed");
        return -1;
    }


    for (int i = 0; i < 8; ++i) {
        trans.img_event_fd[i] = open(img_event_dev_name_[i], O_RDONLY | O_NONBLOCK);
        if (trans.img_event_fd[i] < 0) {
            fprintf(stderr, "unable to open device %s, %d.\n",
            img_event_dev_name_[i], trans.reg_fd);
            perror("open event device failed");
            return -1;
        }
    }

    trans.reg_fd = open(reg_dev_name_, O_RDWR | O_NONBLOCK);
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

    if (trans.h2c_cmd_fd) {
        close(trans.h2c_cmd_fd);
    }
    if (trans.h2c_cmd_fd) {
        close(trans.h2c_cmd_fd);
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

    if (trans.read_cmd_buffer ) {
        free(trans.read_cmd_buffer);
    }

    if (trans.write_cmd_buffer ) {
        free(trans.write_cmd_buffer);
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
        //return -1;
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
    ptr = 0;
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
    if (c2h_fd) {
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

    int max_one_trans = 4096;
    if (size % max_one_trans) {
        size = (1 + (size / max_one_trans)) * max_one_trans;
    }

    for (int i = 0; i < (size / max_one_trans); ++i) {
        memcpy(read_buffer, buffer + (i * max_one_trans), max_one_trans);
        if (h2c_fd) {
            int rc = write_from_buffer(H2C_DEVICE, h2c_fd, read_buffer, 4096, off + (i * max_one_trans));
            if (rc < 0) {
                printf("error index %d, read to buffer failed size %d rc %d\n", 1, size, rc);
                return -1;
            }
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

    auto ret = wait_slv_cmd_ready_event(100);

    if (ret < 0) {
        printf("poll irq error\r\n");
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


static const unsigned char _crc_table[] =
{
    0x00,0x31,0x62,0x53,0xc4,0xf5,0xa6,0x97,0xb9,0x88,0xdb,0xea,0x7d,0x4c,0x1f,0x2e,
    0x43,0x72,0x21,0x10,0x87,0xb6,0xe5,0xd4,0xfa,0xcb,0x98,0xa9,0x3e,0x0f,0x5c,0x6d,
    0x86,0xb7,0xe4,0xd5,0x42,0x73,0x20,0x11,0x3f,0x0e,0x5d,0x6c,0xfb,0xca,0x99,0xa8,
    0xc5,0xf4,0xa7,0x96,0x01,0x30,0x63,0x52,0x7c,0x4d,0x1e,0x2f,0xb8,0x89,0xda,0xeb,
    0x3d,0x0c,0x5f,0x6e,0xf9,0xc8,0x9b,0xaa,0x84,0xb5,0xe6,0xd7,0x40,0x71,0x22,0x13,
    0x7e,0x4f,0x1c,0x2d,0xba,0x8b,0xd8,0xe9,0xc7,0xf6,0xa5,0x94,0x03,0x32,0x61,0x50,
    0xbb,0x8a,0xd9,0xe8,0x7f,0x4e,0x1d,0x2c,0x02,0x33,0x60,0x51,0xc6,0xf7,0xa4,0x95,
    0xf8,0xc9,0x9a,0xab,0x3c,0x0d,0x5e,0x6f,0x41,0x70,0x23,0x12,0x85,0xb4,0xe7,0xd6,
    0x7a,0x4b,0x18,0x29,0xbe,0x8f,0xdc,0xed,0xc3,0xf2,0xa1,0x90,0x07,0x36,0x65,0x54,
    0x39,0x08,0x5b,0x6a,0xfd,0xcc,0x9f,0xae,0x80,0xb1,0xe2,0xd3,0x44,0x75,0x26,0x17,
    0xfc,0xcd,0x9e,0xaf,0x38,0x09,0x5a,0x6b,0x45,0x74,0x27,0x16,0x81,0xb0,0xe3,0xd2,
    0xbf,0x8e,0xdd,0xec,0x7b,0x4a,0x19,0x28,0x06,0x37,0x64,0x55,0xc2,0xf3,0xa0,0x91,
    0x47,0x76,0x25,0x14,0x83,0xb2,0xe1,0xd0,0xfe,0xcf,0x9c,0xad,0x3a,0x0b,0x58,0x69,
    0x04,0x35,0x66,0x57,0xc0,0xf1,0xa2,0x93,0xbd,0x8c,0xdf,0xee,0x79,0x48,0x1b,0x2a,
    0xc1,0xf0,0xa3,0x92,0x05,0x34,0x67,0x56,0x78,0x49,0x1a,0x2b,0xbc,0x8d,0xde,0xef,
    0x82,0xb3,0xe0,0xd1,0x46,0x77,0x24,0x15,0x3b,0x0a,0x59,0x68,0xff,0xce,0x9d,0xac
};

unsigned char calc_crc8(unsigned char *ptr, unsigned int len)
{
    unsigned char crc = 0xaa;

    while (len--)
    {
        crc = _crc_table[crc ^ *ptr++];
    }
    return (crc);
}

int pcie_dev::update_fw(char* bin, int size)
{
    cmd_wr_mutex_.lock();
    pcie_msg_t msg;
    msg.cmd_id = CMD_HOST_UPDATE_QFLASH_FW;
    msg.channel = 0xff;
    msg.data_size = size;
    msg.data_type = ADDR32_AND_SIZE32_INFO;
    msg.crc = calc_crc8((unsigned char *)bin, size);
    printf("cali crc %x\r\n", msg.crc);
    for (int i = 0; i < 4; ++i) {
        msg.data[i] = (PCIE_QFLASH_BIN_MEM_ADDR & (0xff << (i * 8))) >> (i * 8);
    }


    this->pcie_write(trans.h2c_cmd_fd, bin, size, PCIE_QFLASH_BIN_MEM_ADDR, true);
    this->pcie_write(trans.h2c_cmd_fd, (char *)&msg, sizeof(msg), PCIE_POTOCOL_MEM_ADDR, true);
    raise_irq2slv();

    auto ret = wait_slv_cmd_ready_event(20000); //20s

    if (ret < 0) {
        printf("poll irq error\r\n");
        cmd_wr_mutex_.unlock();
        return ret;
    }
    pcie_ack_msg_t ack_msg;
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

    auto ret = wait_slv_cmd_ready_event(100);

    if (ret < 0) {
        printf("poll irq error\r\n");
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
    //if (ack)printf("sdk iic read %x, error code %x\r\n", data, ack_msg.err_code);
    cmd_wr_mutex_.unlock();
    return ack_msg.err_code;
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
