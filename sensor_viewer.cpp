#include "sensor_viewer.h"
#include "ui_sensore_viewer.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <QThread>
#include <QImage>
#include <QTimer>
#include <dma_utils.h>
#include <QFile>
#include <alg_cvtColor.h>
#include <chrono>
#include <iostream>
#include "pcie.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
using namespace chrono;


Sensore_viewer::Sensore_viewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Sensore_viewer)
{
        ui->setupUi(this);

    ui_layout_ = new QGridLayout{};
    for (int i = 0; i < 8; ++i) {
        image_label_[i] = new QLabel{"PCIe Image Label"};
    }


    info_label_ = new QLabel("Decodeinfo");

    QWidget* w = new QWidget;

    statusBar()->addWidget(info_label_);
    ui_layout_->addWidget(image_label_[0], 0, 0, 1, 1);
    ui_layout_->addWidget(image_label_[1], 0, 1, 1, 1);
    ui_layout_->addWidget(image_label_[2], 1, 0, 1, 1);
    ui_layout_->addWidget(image_label_[3], 1, 1, 1, 1);

    w->setLayout(ui_layout_);

    pcie_dev_ = new pcie_dev(0);
    auto ret = pcie_dev_->open_dev();

    image_capture_timer_ = new QTimer;
    image_capture_timer_->setInterval(50);
    connect(image_capture_timer_, &QTimer::timeout, this, &Sensore_viewer::slot_on_sub_ch_image);
    image_capture_timer_->setSingleShot(true);
    image_capture_timer_->start(10);

    //image_capture_thread_->start();
    start_init_camera();
    setCentralWidget(w);
}

char image_chache[1280 * 960 * 3];
char image_chache2[1280 * 960 * 3];
char image_chache_rgb[1280 * 960 * 4];
char image_chache2_rgb[1280 * 960 * 4];

Sensore_viewer::~Sensore_viewer()
{
    image_capture_timer_->stop();

    if (pcie_dev_) {
        delete pcie_dev_;
    }
    delete ui;
}



int Sensore_viewer::start_init_camera()
{
    if (pcie_dev_)
        pcie_dev_->stream_on(NULL, 0);
}

int Sensore_viewer::get_ch_info()
{
    /*int size = 10;
    pcie_msg_t msg;
    msg.cmd_id = 0x10000001;
    memcpy(trans.read_buffer, &msg, sizeof(msg));

    if (trans.c2h_fd) {
        auto rc = read_to_buffer(C2H_DEVICE, trans.c2h_fd, trans.write_buffer, sizeof(msg), 0x70000000 + 2048);
        memcpy(&msg, trans.write_buffer, sizeof(msg));
        if (rc >= 0) {
        info_label_->setText("dt->:" + QString::number(msg.append_info[0], 16) + "|" + QString::number(msg.append_info[1], 16)
                + "|" + QString::number(msg.append_info[2], 16) + "|" + QString::number(msg.append_info[3], 16));
        }
    }*/

}


int Sensore_viewer::read_buffer2image(uchar* image, int size, uint offset)
{
    /*int rc, i;
    int count = COUNT_DEFAULT;
    volatile unsigned int addr;
    volatile unsigned int *transfer_done;
    int var = 0;
    char *write_allocated = trans.write_buffer;
    addr = offset;//0xFAEC00;//0x1869B0;//0x13780;
    int w = 1280;
    int h = 960;
    size = w * h * 2;

    static int index = 0;
    if (0)
        fprintf(stdout, "host buffer 0x%x = %p\n",
            size + 4096, trans.write_buffer);

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
    }

    //save_file("019_raw" +QString::number(++i_index) + ".raw", trans.write_buffer, 1280*960*2);
    alg_cv::cvtColor((uchar*)trans.write_buffer, image, w, h, alg_cv::YUV422_YUYV_2_RGB);
    //yuv422_2_rgb((uchar*)trans.write_buffer, image, w, h);
    */
    return  0;
}

void Sensore_viewer::slot_on_sub_ch_image()
{
    int w = 1280, h = 960;
    int size = 1280 * 960 * 2;
    static int index = 0;

    if (++index > 40) {
        index = 0;
        get_ch_info();
    }

    int ret;


    for (int i = 0; i < 4; ++i) {
        if (pcie_dev_) {
            ret = pcie_dev_->deque_image(image_chache, size, i);
            alg_cv::cvtColor((uchar*)image_chache, (uchar*)image_chache_rgb, w, h, alg_cv::YUV422_YUYV_2_RGB);
        }

        if (!ret) {
            QImage image(reinterpret_cast<uchar *>(image_chache_rgb), w, h, QImage::Format::Format_RGB888);
            float img_scale_h = h * ((float)(image_label_[i]->width()) /(float)(w));
            if (img_scale_h > image_label_[i]->height()) {
                image = image.scaled(w * image_label_[i]->height() / h, (int)image_label_[i]->height());
            } else {
                image = image.scaled(image_label_[i]->width(), h * image_label_[i]->width() / w);
            }

            image_label_[i]->setPixmap(QPixmap::fromImage(image));
        }
    }

    image_capture_timer_->start(20);
}


