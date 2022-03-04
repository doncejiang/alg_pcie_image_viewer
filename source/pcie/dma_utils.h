#ifndef DMA_UTILS_H
#define DMA_UTILS_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  {
    int infile_fd = 0;
    int h2c_fd = 0;
    int c2h_fd = 0;
    int reg_fd = 0;
    int cmd_event_fd = 0;
    int img_event_fd[8]{0};
    int ofile_fd = 0;
    void *map_base{nullptr};
    char *read_buffer{nullptr};
    char *write_buffer{nullptr};
    char *infname{nullptr};
    char *ofname{nullptr};
    uint32_t* map_base_reg{nullptr};
} pcie_transfer_t;

extern ssize_t read_to_buffer(char *fname, int fd, char *buffer, uint64_t size,
            uint64_t base);

extern ssize_t write_from_buffer(char *fname, int fd, char *buffer, uint64_t size,
            uint64_t base);

#ifdef __cplusplus
}
#endif

#endif // DMA_UTILS_H
