#ifndef DMA_UTILS_H
#define DMA_UTILS_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  {
    int infile_fd;
    int h2c_fd;
    int c2h_fd;
    int reg_fd;
    int ofile_fd;
    void *map_base;
    char *read_buffer;
    char *write_buffer;
    char *infname;
    char *ofname;
} pcie_transfer_t;

extern ssize_t read_to_buffer(char *fname, int fd, char *buffer, uint64_t size,
            uint64_t base);

extern ssize_t write_from_buffer(char *fname, int fd, char *buffer, uint64_t size,
            uint64_t base);

#ifdef __cplusplus
}
#endif

#endif // DMA_UTILS_H
