#include "sensor_viewer.h"
#include "ui_sensor_viewer.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <QThread>
#include <QImage>
#include <QTimer>
#include <QFile>
#include <alg_cvtColor.h>
#include <chrono>
#include <iostream>
#include "pcie.h"
#include <sys/stat.h>

#include <sys/types.h>
#include "image_buffer.h"

using namespace std;
using namespace chrono;


sensor_viewer::sensor_viewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::sensor_viewer)
{
        ui->setupUi(this);

    ui_layout_ = new QGridLayout{};

    image_buffer::get_instance(8);

    for (int i = 0; i < 8; ++i) {
        image_label_[i] = new QLabel{"PCIe Image Label"};
    }


    info_label_ = new QLabel("Decodeinfo");

    QWidget* w = new QWidget;

    statusBar()->addWidget(info_label_);
    for (int i = 0; i < 8; ++i) {
        ch_info_label[i] = new QLabel("Channel: " + QString::number(i));
        ch_info_label[i] ->setMaximumHeight(100);
        ui_layout_->addWidget(ch_info_label[i] ,  (i / 4) * 2, i % 4, 1, 1);
        ui_layout_->addWidget(image_label_[i],  (i / 4) * 2 + 1, i % 4, 1, 1);
    }

    w->setLayout(ui_layout_);

    image_capture_timer_ = new QTimer;
    image_capture_timer_->setInterval(50);
    connect(image_capture_timer_, &QTimer::timeout, this, &sensor_viewer::slot_on_sub_ch_image);
    image_capture_timer_->setSingleShot(true);


    image_chache = new char[INPUT_WIDTH_DEFAULT * INPUT_HEIGHT_DEFAULT * 2];
    image_chache_rgb = new char[INPUT_WIDTH_DEFAULT * INPUT_HEIGHT_DEFAULT * 3];

    for (int i = 0; i < 8; ++i) {
        image_capture_process_[i] = new image_capture_proecess(i);
        image_capture_thread_[i] = new QThread;
        image_capture_process_[i]->moveToThread(image_capture_thread_[i]);
        image_capture_thread_[i]->start();

        connect(this, &sensor_viewer::signal_on_start_sensor_stream, image_capture_process_[i], &image_capture_proecess::slot_on_start_sensor_stream);
        connect(this, &sensor_viewer::signal_on_pub_dev_instance, image_capture_process_[i], &image_capture_proecess::slot_on_recv_pcie_dev_control_port);
        connect(image_capture_process_[i], &image_capture_proecess::signal_on_publish_capture_image, this, &sensor_viewer::slot_on_recv_ch_meta_data);
    }

    pcie_dev_ = new pcie_dev(0);
    auto ret = pcie_dev_->open_dev();


    /*while (1) {
        printf("wait trg\r\n");
        pcie_dev_->wait_slv_cmd_ready_event(1000);
        printf("trg recv");
    }*/
    if (ret == 0) {
        //start_init_camera();
        //QThread::msleep(10000);
        emit signal_on_pub_dev_instance(pcie_dev_);
        image_capture_timer_->start(10);
        emit signal_on_start_sensor_stream();
    }
    setCentralWidget(w);
}

sensor_viewer::~sensor_viewer()
{
extern bool g_stop_capture_sensor_stream;
    g_stop_capture_sensor_stream = true;

    for (int i = 0; i < 8; ++i) {
        if (image_capture_thread_[i]) {
            image_capture_thread_[i]->wait(500);
             if (image_capture_thread_[i]->isRunning()) {
                image_capture_thread_[i]->terminate();
                 image_capture_thread_[i]->deleteLater();
            }
        }
    }

    if (pcie_dev_) {
        delete pcie_dev_;
    }

    delete []  image_chache;
    delete [] image_chache_rgb;

    delete ui;
}



int sensor_viewer::start_init_camera()
{
    if (pcie_dev_) pcie_dev_->stream_on(NULL, 0);
    return 0;
}

int sensor_viewer::read_buffer2image(uchar* image, int size, uint offset)
{/*
    auto start = system_clock::now();
    if (trans.c2h_fd) {
        rc = read_to_buffer(C2H_DEVICE, trans.c2h_fd, trans.write_buffer, size, addr);
        if (rc < 0) {
            printf("error index %d, read to buffer failed size %d rc %d\n", index, size, rc);
            return -1;
        }
    }


    if (++index % 60 == 0) {
        auto end   = system_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        std::cout <<  "花费了"
             << double(duration.count()) * microseconds::period::num / microseconds::period::den
             << "秒 Badnwidth " << size / 1024 / 1024 / (double(duration.count()) * microseconds::period::num / microseconds::period::den) << "MB/s" << endl;
    }*/

    return  0;
}
static hw_sts hw_sts_;
void sensor_viewer::slot_on_sub_ch_image()
{
    if (pcie_dev_) {
        //auto start_tick   = system_clock::now();
        if (!pcie_dev_->get_channel_decode_info(hw_sts_)) {
           // auto end_tick   = system_clock::now();
            //auto duration = duration_cast<microseconds>(end_tick - start_tick);
            //printf ("花费了 %f ms", double(duration.count()) * microseconds::period::num / microseconds::period::den);
            QString str = "ch_dt:";
            for (int j = 0; j < 8; ++j) {
                str += QString::number(hw_sts_.dt[j], 16) + "|";
            }
            info_label_->setText(str);
            //try read iic info

            //
            printf ("hw_sts_ vol %f %f %f %f\r\n", hw_sts_.vol[0], hw_sts_.vol[1], hw_sts_.vol[2], hw_sts_.vol[3]);
            printf ("hw_sts_ cur %f %f %f %f\r\n", hw_sts_.cur[0], hw_sts_.cur[1], hw_sts_.cur[2], hw_sts_.cur[3]);
            uint16_t data;
            //auto ret = pcie_dev_->i2c_read(0, 0x90, 0x00, data, 0x1608);printf("read data %x, code %x\r\n", data, ret);
        }
    }
    image_capture_timer_->start(1000);
}


void sensor_viewer::slot_on_recv_ch_meta_data(void* meta_data, int ch_id)
{
    if (ch_id > 8 || !meta_data) return;

        image_meta_data_t* data;
        image_buffer::get_instance()->deque(&data, ch_id);
        image_meta_data_t *image_meta_data = (image_meta_data_t *)meta_data;


        QImage view_image(image_meta_data->rgb_data.data, image_meta_data->image_info.width, image_meta_data->image_info.height, QImage::Format_RGB888);
        QImage tmp;

        float image_scale_height = (float)image_meta_data->image_info.height * ( (float)image_label_[ch_id]->size().width() / (float)image_meta_data->image_info.width);

        //说明height按照 viewer来
        if (image_scale_height >= image_label_[ch_id]->size().height()) {
            tmp = view_image.scaled(QSize(image_meta_data->image_info.width * image_label_[ch_id]->size().height() / image_meta_data->image_info.height,
                                          image_label_[ch_id]->size().height()));
        }else { //说明 width 按照viewer来
            tmp = view_image.scaled(QSize(image_label_[ch_id]->size().width(),
                                          image_meta_data->image_info.height *  image_label_[ch_id]->size().width()/ image_meta_data->image_info.width));
        }

        QString str = ("|Vol:" + QString::number(hw_sts_.vol[ch_id], 'g', 6) + "V|Cur:" + QString::number(hw_sts_.cur[ch_id], 'g', 6) + "mA");
        ch_info_label[ch_id]->setText("Channel" + QString::number(ch_id) + "_FPS: " + QString::number(image_meta_data->image_info.fps) + str);

        this->image_label_[ch_id]->setPixmap(QPixmap::fromImage(tmp));
}
