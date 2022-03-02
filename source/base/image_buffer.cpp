#include "image_buffer.h"
#include <QDebug>


ring_buffer::ring_buffer(int item_size, int buffer_depth, item_init init, item_deinit deinit, QObject *parent)
{
     meta_data_ = new char[item_size * buffer_depth];
     if (init) {
         item_init_func = init;
         for (int i = 0; i < buffer_depth; ++i) {
             //qDebug("ch %d addr %x start init item", i, (void *)((char *)meta_data_ + (i * item_size)));
             item_init_func((void *)((char *)meta_data_ + (i * item_size)));
         }
     }
     if (deinit) {
         item_deinit_func = deinit;
     }

     buffer_depth_ = buffer_depth;
     item_size_ = item_size;
     write_index_ = 0;
     read_index_ = 0;
}

ring_buffer::~ring_buffer()
{
    for (int i = 0; i < buffer_depth_; ++i) {
       item_deinit_func((void *)((char *)meta_data_ + (i * item_size_)));
    }
}

error_status_t ring_buffer::deque(void** data)
{
    std::lock_guard<std::mutex> lck(mutex_);
    if (is_empty() || !data) {
        return  STATUS_ERROR;
    }
    *data = (void *)((char *)meta_data_ + (read_index_ * item_size_));
    read_index_ =(read_index_ + 1) % buffer_depth_;
    return STATUS_OK;
}

error_status_t ring_buffer::enque(void** data)
{
    std::lock_guard<std::mutex> lck(mutex_);
    if (is_full() || !data) {
        qDebug("ring buffer full");
        return  STATUS_ERROR;
    }
    *data = (void *)((char *)meta_data_ + (write_index_ * item_size_));
     write_index_ = (write_index_ + 1) % buffer_depth_;
     return STATUS_OK;
}

bool image_meta_data_init(void* item_ptr)
{
    image_meta_data_t *meta_data = static_cast<image_meta_data_t *>(item_ptr);

    memset(item_ptr, 0, sizeof(image_meta_data_t));

    strcpy(meta_data->raw_data.desc, "raw_image_buffer");
    meta_data->raw_data.size = MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 2;
    meta_data->raw_data.data = new uint8_t[meta_data->raw_data.size];
    if (!meta_data->raw_data.data) return false;
    //qDebug("raw addr %x", meta_data->raw_data.data );

    //meta_data->rgb_data.desc = "rgb_image_buffer";
    strcpy(meta_data->rgb_data.desc, "rgb_image_buffer");
    meta_data->rgb_data.size = MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 3;
    meta_data->rgb_data.data = new uint8_t[meta_data->rgb_data.size];
    if (!meta_data->rgb_data.data) {
        delete [] meta_data->raw_data.data;
        return false;
    }

    //meta_data->yuv_data.desc = "yuv_image_buffer";
    strcpy(meta_data->yuv_data.desc, "yuv_image_buffer");
    meta_data->yuv_data.size = MAX_IMAGE_WIDTH * MAX_IMAGE_HEIGHT * 2;
    meta_data->yuv_data.data = new uint8_t[meta_data->yuv_data.size];
    if (!meta_data->yuv_data.data) {
        delete [] meta_data->raw_data.data;
        delete [] meta_data->rgb_data.data;
        return false;
    }

    return true;
}

bool image_meta_data_deinit(void* item_ptr)
{
    image_meta_data_t *meta_data = static_cast<image_meta_data_t *>(item_ptr);

    if (meta_data->raw_data.data) {
        delete [] meta_data->raw_data.data;
    }

    if (meta_data->rgb_data.data) {
        delete [] meta_data->rgb_data.data;
    }

    if (meta_data->yuv_data.data) {
        delete [] meta_data->yuv_data.data;
    }
    //qDebug() << "delete buffer";

    return true;
}

static image_buffer*  s_image_buffer_ptr = nullptr;

image_buffer::image_buffer(int ch_num, QObject *parent) : QObject(parent)
{
    ch_num_ = ch_num;
    for (int i = 0; i < ch_num; ++i) {
        image_buffer_queue_[i] = new ring_buffer(sizeof(image_meta_data_t), DEF_BUFFER_DEPTH, image_meta_data_init, image_meta_data_deinit);
    }
}


image_buffer::~image_buffer()
{
    for (int i = 0; i < ch_num_; ++i) {
        delete image_buffer_queue_[i];
    }
}


image_buffer*  image_buffer::get_instance(int ch_num)
{
    if (!s_image_buffer_ptr) {
        s_image_buffer_ptr = new image_buffer(ch_num);
    }
    return s_image_buffer_ptr;
}

void image_buffer::release()
{
    if (s_image_buffer_ptr) {
        delete s_image_buffer_ptr;
        s_image_buffer_ptr = nullptr;
    }
}

error_status_t image_buffer::deque(image_meta_data_t** image_data, int ch_id)
{
    if (ch_id < ch_num_ && image_buffer_queue_[ch_id]) {
        auto sts = image_buffer_queue_[ch_id]->deque((void **)(image_data));
        //qDebug("deque %x", *image_data);
    }
    return STATUS_ERROR;
}
error_status_t image_buffer::enque(image_meta_data_t** image_data, int ch_id)
{
    if (ch_id < ch_num_ && image_buffer_queue_[ch_id]) {
        void *data = nullptr;
        auto sts = image_buffer_queue_[ch_id]->enque((void **)(image_data));
        //qDebug("enque %x", *image_data);
    }
    return STATUS_ERROR;
}

