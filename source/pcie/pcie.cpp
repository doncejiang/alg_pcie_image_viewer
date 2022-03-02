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
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "stdlib.h"
#include "string.h"

#define MAP_SIZE (32*1024UL)
#define MAP_MASK (MAP_SIZE - 1)

#define H2C_DEVICE "/dev/xdma%d_h2c_0"
#define C2H_DEVICE "/dev/xdma%d_c2h_0"
#define REG_DEVICE_NAME "/dev/xdma%d_xvc"

#define MEM_ALLOC_SIZE (3840 * 2166 * 2)

pcie_dev::pcie_dev(int dev_id)
{
    this->dev_id_ = dev_id;

    sprintf(c2h_dev_name_, C2H_DEVICE, dev_id);
    sprintf(h2c_dev_name_, H2C_DEVICE, dev_id);
    sprintf(reg_dev_name_, REG_DEVICE_NAME, dev_id);

    for (int i = 0; i < VDMA_NUM; ++i) {
        for (int j = 0; j < VDMA_RING_FRM_NUM; ++j) {
            addr_table_[i][j] = i * VDMA_RING_FRM_NUM * (1920 * 1090 * 2) + ((1280 * 2 * 1000) * j) + PCIE_IMAGE_MEM_ADDR;
        }
    }
}

pcie_dev::~pcie_dev()
{
    if (dev_is_open_) {
        close_dev();
    }
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

    trans.read_buffer = read_allocated;
    posix_memalign((void **)&write_allocated, 4096 , alloc_size + 4096);
    if (!write_allocated) {
        fprintf(stderr, "OOM %u.\n", alloc_size + 4096);
    }

    trans.write_buffer = write_allocated;

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

    if (trans.reg_fd) {
        if (munmap(trans.map_base, MAP_SIZE) == -1)
            printf("error unmap\n");
        close(trans.reg_fd);
    }

    if (trans.read_buffer ) {
        free(trans.read_buffer);
    }

    if (trans.write_buffer ) {
        free(trans.write_buffer);
    }

    dev_is_open_ = false;
}

int pcie_dev::stream_on(void* cfg, uint8_t channel)
{
    pcie_msg_t msg;
    msg.cmd_id = 0x10000001;
    msg.channel = channel;
    memcpy(trans.read_buffer, &msg, sizeof(msg));

    if (trans.h2c_fd) {
        int rc = write_from_buffer(h2c_dev_name_, trans.h2c_fd, trans.read_buffer, sizeof(msg), PCIE_POTOCOL_MEM_ADDR);
        if (rc < 0) {
            printf("stream on read to buffer failed size %d rc %d\n", sizeof(msg), rc);
            return -1;
        }
        printf("raise irq\r\n");
        raise_irq2slv();
        //if (trans.map_base_reg) {
            //trans.map_base_reg[11] = 1; //raise irq
        //}
    }

    return 0;
}

int pcie_dev::stream_off(uint8_t channel)
{
    return 0;
}

int pcie_dev::get_decode_info(char* buffer, size_t size)
{
    pcie_msg_t msg;

    if (trans.c2h_fd) {
        auto rc = read_to_buffer(c2h_dev_name_, trans.c2h_fd, trans.write_buffer, sizeof(msg), PCIE_POTOCOL_MEM_ADDR + 2048);
        memcpy(&msg, trans.write_buffer, sizeof(msg));
        return rc;
    }
    return -1;
}

int pcie_dev::deque_image(char* image, uint32_t size, uint8_t channel)
{
    if (channel > VDMA_NUM || !image) return -1;

    static uint8_t s_frm7_grey2dec_lut[16] = {0xff, 0, 2, 1,  6, 5, 3, 4, 0xff, 6,  4, 5,  0,  1,  3,  2};
    auto grey_code = get_frm_ptr(channel);
    if (grey_code > 15) grey_code = 15;
    auto ptr = s_frm7_grey2dec_lut[grey_code];
    if (ptr > 6) ptr = 6;

    if (ptr == 0){
        ptr = 6;
    } else {
        --ptr;
    }

    return read(image, size, addr_table_[channel][ptr]);
}

size_t pcie_dev::read(char* buffer, size_t size, size_t off)
{
    uint64_t addr = off;

    if (trans.c2h_fd) {
        auto rc = read_to_buffer(c2h_dev_name_, trans.c2h_fd, trans.write_buffer, size, addr);
        if (rc < 0) {
            printf("dev %d read to buffer failed size %d rc %d\n", dev_id_, size, rc);
            return -1;
        }
        memcpy(buffer, trans.write_buffer, size);
    }
    return 0;
}

size_t pcie_dev::write(char* buffer, size_t size, size_t off)
{
    memcpy(trans.read_buffer, buffer, size);
    if (trans.h2c_fd) {
        int rc = write_from_buffer(H2C_DEVICE, trans.h2c_fd, trans.read_buffer, size, off);
        if (rc < 0) {
            printf("error index %d, read to buffer failed size %d rc %d\n", 1, size, rc);
            return -1;
        }
    }
    return 0;
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
