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
    char* write_dev_buffer{nullptr};
    char* read_dev_buffer{nullptr};
};


int xdma_read_device(HANDLE device, long address, DWORD size, BYTE* buffer);
int xdma_open_device(struct pcie_trans* trans, int dev_id);
int xdma_write_device(HANDLE device, long address, DWORD size, BYTE* buffer);
int xdma_close_device(struct pcie_trans* trans, int dev_id);

#ifdef __cplusplus
}
#endif


#endif // XDMA_DRV_H
