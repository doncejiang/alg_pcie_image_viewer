#ifndef XDMA_DRV_H
#define XDMA_DRV_H

#ifdef __cplusplus
 extern "C" {
 #endif

#include <windows.h>

#define VDMA_NUM 8
#define VDMA_RING_FRM_NUM 6


struct pcie_trans {
    HANDLE c2h0_device{nullptr};
    HANDLE h2c0_device{nullptr};
    HANDLE user_device{nullptr};
    HANDLE img_event_device[8]{nullptr};
    HANDLE cmd_event_device{nullptr};
    char* read_img_buffer[VDMA_NUM]{nullptr};
    char* write_dev_buffer[VDMA_NUM]{nullptr};
    char* read_dev_buffer[VDMA_NUM]{nullptr};
};

void put_pic_to_sys_memory(char *path);
int pcie_init();
void pcie_deinit();
unsigned int h2c_transfer(unsigned int size);
unsigned int c2h_transfer(unsigned int size);
int wait_for_s2mm_intr();
int wait_for_mm2s_intr();
int disp_init(int *width,int *height,char *pic_name);
void disp_deinit();
void disp_start();
void disp_reset();
int pcie_read_frame(unsigned char *buffer);
void set_saturation(unsigned char satu);


#ifdef __cplusplus
}
#endif


#endif // XDMA_DRV_H
