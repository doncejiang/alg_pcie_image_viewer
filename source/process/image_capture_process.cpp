#include "image_capture_process.h"
#include <app_cfg.h>
#include <QThread>
#include <alg_cvtColor.h>
#include <QMutex>
#include <QDateTime>
#include <math.h>
#include <sys/time.h>

QMutex mutex;
bool g_stop_capture_sensor_stream = false;

image_capture_proecess::image_capture_proecess(int ch_id, QObject *parent) : QObject(parent)
{
    ch_id_ = ch_id;
}

void image_capture_proecess::slot_on_recv_pcie_dev_control_port(void* dev_base)
{
    pcie_dev_ = (pcie_dev *)dev_base;
}

uint32_t get_ms_tick(struct timeval tv)
{
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void image_capture_proecess::slot_on_start_sensor_stream()
{
    qDebug("ch %d start capture", ch_id_);
    int err_cnt_ = 0;
    int frame_cnt = 0;
    struct timeval tv;
    struct timeval last_tv;
    gettimeofday(&tv, NULL);
    bool is_last_error = false;
    int fps = 0;

    while (!g_stop_capture_sensor_stream) {
        //capture
        //wait signal, tmp use sleep
        //if (ch_id_ == 0) {
        //    auto rc = pcie_dev_->wait_slv_cmd_ready_event();
        //    printf("wait rc %x\r\n", rc);
        //} else {
        //    QThread::msleep(10); // 25fps
        //}

        QThread::msleep(5);
        //auto rc = pcie_dev_->wait_image_ready_event(ch_id_);
        //if (rc < 0) {
        //    printf("ch %d wait event failed %d\r\n", ch_id_, rc);
        //}
        if (pcie_dev_) {
            if (!is_last_error)
                image_buffer::get_instance()->enque(&meta_data_, ch_id_);
            //TODO: auto fit
            if (ch_id_ == 4 || ch_id_ == 5 || ch_id_ == 1 || ch_id_ == 0) {
                meta_data_->image_info.width = 1920;
                meta_data_->image_info.height = 1280;
            } else {
                meta_data_->image_info.width = get_app_cfg_info()->width;
                meta_data_->image_info.height = get_app_cfg_info()->height;
            }
            //mutex.lock();
            int ret = pcie_dev_->deque_image((char *)meta_data_->raw_data.data,
                                             meta_data_->image_info.width * meta_data_->image_info.height * 2, ch_id_);
            //mutex.unlock();

            alg_cv::ALG_cvtColor((uchar*)meta_data_->raw_data.data, (uchar*)meta_data_->rgb_data.data,
                             meta_data_->image_info.width, meta_data_->image_info.height, alg_cv::YUV422_YUYV_2_RGB);
            meta_data_->image_info.soft_frame_index = ++frame_index_;
            if (ret >= 0)
            {
                is_last_error = false;
                err_cnt_ = 0;
                ++frame_cnt;
                gettimeofday(&tv, NULL);
                if ((get_ms_tick(tv) - get_ms_tick(last_tv)) >= 1000) {
                    fps = (frame_cnt * 10000) / (get_ms_tick(tv) - get_ms_tick(last_tv));
                    auto tmp = fps % 10;
                    if (tmp > 5) fps = fps / 10 + 1;
                    else fps = fps / 10;

                    meta_data_->image_info.fps = fps;
                    printf("ch %d-> %d fps\r\n", ch_id_, meta_data_->image_info.fps);
                    last_tv = tv;
                    frame_cnt = 0;
                }
                if (meta_data_->image_info.fps < 5) meta_data_->image_info.fps = fps;
                if (frame_cnt > 1) {
                    emit signal_on_publish_capture_image(meta_data_, ch_id_);
                }
            } else {
                is_last_error = true;
                ++err_cnt_;
            }
            if (err_cnt_ > 20) {
                 printf("ch %d quit beacase error\r\n", ch_id_);
                 break;
            }
        }
    }

    qDebug("ch %d capture process quit", ch_id_);
}
