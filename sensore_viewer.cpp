#include "sensore_viewer.h"
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
#include "pcie_protocol.h"

using namespace std;
using namespace chrono;

pcie_transfer_t trans;

inline void pice_close(int fd) {
    close(fd);
}

Sensore_viewer::Sensore_viewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Sensore_viewer)
{
        ui->setupUi(this);

    ui_layout_ = new QGridLayout{};
    for (int i = 0; i < 8; ++i) {
        image_label_[i] = new QLabel{"PCIe Image Label"};
    }

    QWidget* w = new QWidget;



    ui_layout_->addWidget(image_label_[0], 0, 0, 1, 1);
    ui_layout_->addWidget(image_label_[1], 0, 1, 1, 1);
    ui_layout_->addWidget(image_label_[2], 1, 0, 1, 1);
    ui_layout_->addWidget(image_label_[3], 1, 1, 1, 1);

    w->setLayout(ui_layout_);

    char *h2c_device = H2C_DEVICE;
    char *c2h_device = C2H_DEVICE;

    trans.h2c_fd = open(h2c_device, O_RDWR);
    if (trans.h2c_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            h2c_device, trans.h2c_fd);
        perror("open device");
        return;
    }

    trans.c2h_fd = open(c2h_device, O_RDWR);
    if (trans.c2h_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            c2h_device, trans.c2h_fd);
        perror("open device");
        return;
    }


     char *read_allocated = nullptr;
     char *write_allocated = nullptr;
     constexpr size_t alloc_size = 1280 * 720 * 3;

     posix_memalign((void **)&read_allocated, 4096 , alloc_size + 4096);
     if (!read_allocated) {
         fprintf(stderr, "OOM %u.\n", alloc_size + 4096);
     }

    trans.read_buffer = read_allocated;
    posix_memalign((void **)&write_allocated, 4096 , alloc_size + 4096);
    if (!write_allocated) {
        fprintf(stderr, "OOM %u.\n", alloc_size + 4096);
    }

    trans.write_buffer = write_allocated;

    //image_capture_process_ = new image_capture_process{};
    //image_capture_thread_ = new QThread{};

    //image_capture_process_->moveToThread(image_capture_thread_);

    image_capture_timer_ = new QTimer;
    image_capture_timer_->setInterval(50);
    connect(image_capture_timer_, &QTimer::timeout, this, &Sensore_viewer::slot_on_sub_ch_image);
    image_capture_timer_->setSingleShot(true);
    image_capture_timer_->start(10);

    //connect()

    //image_capture_thread_->start();


    start_init_camera();
    setCentralWidget(w);
}

uchar image_chache[1280 * 960 * 3];
uchar image_chache2[1280 * 960 * 3];
uchar image_chache_rgb[1280 * 960 * 4];
uchar image_chache2_rgb[1280 * 960 * 4];

Sensore_viewer::~Sensore_viewer()
{
    image_capture_timer_->stop();

    if (trans.h2c_fd) {
        pice_close(trans.h2c_fd);
    }
    if (trans.h2c_fd) {
        pice_close(trans.h2c_fd);
    }

    if (trans.read_buffer ) {
        free(trans.read_buffer);
    }

    if (trans.write_buffer ) {
        free(trans.write_buffer);
    }

    delete ui;
}



int Sensore_viewer::start_init_camera()
{
    int size = 10;
    pcie_msg_t msg;
    msg.cmd_id = 0x10000001;
    memcpy(trans.read_buffer, &msg, sizeof(msg));

    if (trans.h2c_fd) {
        int rc = write_from_buffer(H2C_DEVICE, trans.h2c_fd, trans.read_buffer, sizeof(msg), 0x70000000);
        if (rc < 0) {
            printf("error index %d, read to buffer failed size %d rc %d\n", 1, size, rc);
            return -1;
        } else {
            //printf("rc %d addr %x data [0-3] %d %d %d %d\r\n",  rc, addr, write_allocated[0], write_allocated[1], write_allocated[2], write_allocated[3]);
        }
    }
}
int Sensore_viewer::read_buffer2image(uchar* image, int size, uint offset)
{
    int rc, i;
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
        } else {
            //printf("rc %d addr %x data [0-3] %d %d %d %d\r\n",  rc, addr, write_allocated[0], write_allocated[1], write_allocated[2], write_allocated[3]);
        }
    }


    if (++index % 30 == 0) {
        auto end   = system_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        std::cout <<  "花费了"
             << double(duration.count()) * microseconds::period::num / microseconds::period::den
             << "秒 Badnwidth " << size / 1024 / 1024 / (double(duration.count()) * microseconds::period::num / microseconds::period::den) << "MB/s" << endl;
    }

    //save_file("019_raw" +QString::number(++i_index) + ".raw", trans.write_buffer, 1280*960*2);
    alg_cv::cvtColor((uchar*)trans.write_buffer, image, w, h, alg_cv::YUV422_YUYV_2_RGB);
    //yuv422_2_rgb((uchar*)trans.write_buffer, image, w, h);
    return  0;
}


void Sensore_viewer::slot_on_sub_ch_image()
{
    int w = 1280, h = 960;
    int size = 1280 * 960 * 2;

    int ret = read_buffer2image(image_chache_rgb, size, 0x70001000);

    if (!ret) {
       // yuv422_2_rgb(image_chache, image_chache_rgb, w, h);
        QImage image(reinterpret_cast<uchar *>(image_chache_rgb), w, h, QImage::Format::Format_RGB888);
        image = image.scaled(image_label_[0]->width(), (int)image_label_[0]->height());
        image_label_[0]->setPixmap(QPixmap::fromImage(image));
    }

     ret = read_buffer2image(image_chache2_rgb, size, 0x711cd400);
    if (!ret) {
        //yuv422_2_rgb(image_chache2, image_chache2_rgb, w, h);
        QImage image2(reinterpret_cast<uchar *>(image_chache2_rgb), w, h, QImage::Format::Format_RGB888);
        image2 = image2.scaled(image_label_[1]->width(), (int)image_label_[1]->height());
        image_label_[1]->setPixmap(QPixmap::fromImage(image2));
    }


    /*ret = read_buffer2image(image_chache_rgb, size, 0xfafc00);

    if (!ret) {
       // yuv422_2_rgb(image_chache, image_chache_rgb, w, h);
        QImage image(reinterpret_cast<uchar *>(image_chache_rgb), w, h, QImage::Format::Format_RGB888);
        image = image.scaled(640, 480);
        image_label_[2]->setPixmap(QPixmap::fromImage(image));
    }

    ret = read_buffer2image(image_chache2_rgb, size, 0x2bd5600);
    if (!ret) {
        //yuv422_2_rgb(image_chache2, image_chache2_rgb, w, h);
        QImage image2(reinterpret_cast<uchar *>(image_chache2_rgb), w, h, QImage::Format::Format_RGB888);
        image2 = image2.scaled(640, 480);
        image_label_[3]->setPixmap(QPixmap::fromImage(image2));
    }*/

    image_capture_timer_->start(20);
}


