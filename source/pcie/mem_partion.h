#ifndef APP_MEM_H
#define APP_MEM_H

#define PCIE_QFLASH_BIN_MEM_ADDR (0x36000000)
//#define PCIE_QFLASH_BIN_WRITE_ADDR (0x36000000)
#define PCIE_QFLASH_BIN_VERFY_MEM_ADDR (0x38000000)
#define PCIE_POTOCOL_MEM_ADDR (0x40000000)
#define PCIE_POTOCOL_MEM_SIZE (512u)
#define PCIE_POTOCOL_HOST_CMD_BIG_DATA_MEM_SIZE (4096u) //4KB data
#define PCIE_POTOCOL_SLV_CMD_BIG_DATA_MEM_SIZE (4096u)  //4KB data


#define PCIE_IMAGE_MEM_ADDR (PCIE_POTOCOL_MEM_ADDR + PCIE_POTOCOL_MEM_SIZE + PCIE_POTOCOL_HOST_CMD_BIG_DATA_MEM_SIZE + PCIE_POTOCOL_SLV_CMD_BIG_DATA_MEM_SIZE)
#define PCIE_IMAGE_MEM_SZIE (0x7FF00000 - PCIE_POTOCOL_MEM_SIZE)

#define PCIE_PROTOCOL_CMD_MAX_LEN 128
//#define PCIE_PROTOCOL_EXTREA_MAX_LEN 256

#define PCIE_POTOCOL_HOST_CMD_MEM_ADDR PCIE_POTOCOL_MEM_ADDR   //0x400000000 //128B
#define PCIE_POTOCOL_HOST_CMD_ACK_MEM_ADDR (PCIE_POTOCOL_MEM_ADDR + PCIE_PROTOCOL_CMD_MAX_LEN)  //128

#define PCIE_POTOCOL_SLV_CMD_MEM_ADDR (PCIE_POTOCOL_HOST_CMD_ACK_MEM_ADDR + PCIE_PROTOCOL_CMD_MAX_LEN) //128
#define PCIE_POTOCOL_SLV_CMD_ACK_MEM_ADDR (PCIE_POTOCOL_SLV_CMD_MEM_ADDR + PCIE_PROTOCOL_CMD_MAX_LEN) //128Byte

#define PCIE_POTOCOL_HOST_EXTRA_MEM_ADDR (PCIE_POTOCOL_SLV_CMD_ACK_MEM_ADDR + PCIE_POTOCOL_HOST_CMD_BIG_DATA_MEM_SIZE) //4KB
#define PCIE_POTOCOL_SLV_EXTRA_MEM_ADDR  (PCIE_POTOCOL_HOST_EXTRA_MEM_ADDR + PCIE_POTOCOL_SLV_CMD_BIG_DATA_MEM_SIZE) //4K

#endif
