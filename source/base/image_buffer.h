#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H

#include <app_config.h>
#include <mutex>

typedef struct {
    char     sensor_name[32];
    uint32_t frame_index;
    uint32_t soft_frame_index = 0;
    uint32_t time_stamp;
    uint8_t  data_type;
    uint8_t  vc_id;
    uint16_t width;
    uint16_t height;
    int    fps;
    uint32_t rsvd;
} image_info_t;

typedef struct {
    uint8_t* data;
    uint32_t size;
    char     desc[32];
} image_data_t;

typedef struct {
    image_info_t image_info;
    image_data_t raw_data;
    image_data_t rgb_data;
    image_data_t yuv_data;
} image_meta_data_t;


typedef bool (*item_init)(void* item_ptr);
typedef bool (*item_deinit)(void* item_ptr);

class ring_buffer
{
public:
    explicit ring_buffer(int item_size, int buffer_depth, item_init init, item_deinit deinit);
    ~ring_buffer();
    alg_error_status_t deque(void** data);
    alg_error_status_t enque(void** data);
    bool is_full() {
        return (write_index_ + 1) % buffer_depth_== read_index_;
    }
    bool is_empty() {
        return read_index_ == write_index_;
    }
private:
    int write_index_ = 0;
    int read_index_ = 0;
    int buffer_depth_ = 0;
    int item_size_ = 0;
    std::mutex wr_mutex_;
    void* meta_data_ = nullptr;
    item_init item_init_func = nullptr;
    item_deinit item_deinit_func = nullptr;
    std::mutex mutex_;
};

//TODO:add multi thread support
class image_buffer
{
public:
    ~image_buffer();
    static image_buffer*  get_instance(int ch_num = MAX_DEV_CH_NUM);
    static void    release();
    alg_error_status_t deque(image_meta_data_t** image_data, int ch_id);
    alg_error_status_t enque(image_meta_data_t** image_data, int ch_id);
private:
    explicit image_buffer(int ch_num = MAX_DEV_CH_NUM);
    ring_buffer*   image_buffer_queue_[MAX_DEV_CH_NUM] = {nullptr};
    int  ch_num_ = 0;
};

#endif // IMAGE_BUFFER_H
