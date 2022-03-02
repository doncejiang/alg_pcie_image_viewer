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
}

pcie_dev::~pcie_dev()
{
    if (dev_is_open_) {
        close_dev();
    }
}

int pcie_dev::open_dev()
{
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

size_t pcie_dev::read(char* buffer, size_t size, size_t off)
{
    uint64_t addr = off;

    if (trans.c2h_fd) {
        auto rc = read_to_buffer(c2h_dev_name_, trans.c2h_fd, trans.write_buffer, size, addr);
        if (rc < 0) {
            printf("dev %d read to buffer failed size %d rc %d\n", dev_id_, size, rc);
            return -1;
        }
    }
}

size_t pcie_dev::write(char* buffer, size_t size, size_t off)
{
    if (trans.h2c_fd) {
        int rc = write_from_buffer(H2C_DEVICE, trans.h2c_fd, trans.read_buffer, size, off);
        if (rc < 0) {
            printf("error index %d, read to buffer failed size %d rc %d\n", 1, size, rc);
            return -1;
        }
    }
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
