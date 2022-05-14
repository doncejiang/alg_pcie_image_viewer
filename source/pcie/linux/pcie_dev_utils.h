#ifndef DMA_UTILS_H
#define DMA_UTILS_H

#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>

#define MAX_PCIE_DEV_NUM 2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  {
    int h2c_img_fd;
    int c2h_img_fd;
    int h2c_cmd_fd;  //cmd
    int c2h_cmd_fd; //cmd
    int dev_ctl_reg_fd;
    int host_cmd_ack_event_fd;
    int soc_cmd_event_fd;
    int img_refresh_event_fd;
} pcie_clt_handle_t;

typedef struct mem_item
{
    int index;
    void *buf;
    size_t size;
    int reserved;
} mem_item_t;

//for read and write
typedef struct mem_rw_queue
{
    mem_item_t* items;
    size_t buf_number;

    int r_index;
    int w_index;
    
    pthread_mutex_t wr_mutex;
    //interface
    int (*queue_init)(mem_rw_queue_t* queue, size_t number,  size_t item_content_size);
    int (*deque)(mem_rw_queue_t* queue, mem_item_t**  item ,  uint time_out);
    int (*enque)(mem_rw_queue_t* queue,  mem_item_t* item, uint timeout);
    int (*queue_deinit)(mem_rw_queue_t* queue);
} mem_rw_queue_t;

//for only write
typedef struct mem_buf
{
    mem_item_t* items;
    size_t buf_number;
} mem_buf_t;

typedef struct pcie_dev_mem
{
    mem_buf_t rd_cmd_mem;
    mem_buf_t wr_cmd_mem;
    mem_rw_queue_t img_queue;
    void *map_base_addr;
} pcie_dev_mem_t;


typedef enum  dev_sts {
    CLOSED =  0,
    OPENED = 1,
} dev_sts_e;


typedef struct dev_info
{
    #define MAX_VERISON_NAME 128
    char sdk_version[MAX_VERISON_NAME];
    char dev_name[MAX_VERISON_NAME];
    size_t max_channel;
    int rsvd;
} dev_info_t;
 

typedef struct pcie_sdk_handle
{
    dev_info_t                    info[MAX_PCIE_DEV_NUM];
    dev_sts_e                      sts[MAX_PCIE_DEV_NUM];
    pcie_dev_mem_t      dev_mem[MAX_PCIE_DEV_NUM];
    pcie_clt_handle_t     clt_handle[MAX_PCIE_DEV_NUM];
    size_t                               dev_num;
} pcie_sdk_handle_t;



extern ssize_t read_to_buffer(char *fname, int fd, char *buffer, uint64_t size,
            uint64_t base);

extern ssize_t write_from_buffer(char *fname, int fd, char *buffer, uint64_t size,
            uint64_t base);

#ifdef __cplusplus
}
#endif

#endif // DMA_UTILS_H
