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

pcie_transfer_t trans;

inline void pice_close(int fd) {
    close(fd);
}

Sensore_viewer::Sensore_viewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Sensore_viewer)
{
    ui_layout_ = new QGridLayout{};
    image_label_ = new QLabel{"PCIe Image Label"};

    this->layout()->addWidget(image_label_);
    setLayout(ui_layout_);

    char *h2c_device = H2C_DEVICE;
    char *c2h_device = C2H_DEVICE;

    trans.h2c_fd = open(h2c_device, O_RDWR);
    if (trans.h2c_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            h2c_device, trans.h2c_fd);
        perror("open device");
    }

    trans.c2h_fd = open(c2h_device, O_RDWR);
    if (trans.c2h_fd < 0) {
        fprintf(stderr, "unable to open device %s, %d.\n",
            c2h_device, trans.c2h_fd);
        perror("open device");
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
    image_capture_timer_->setInterval(30);
    connect(image_capture_timer_, &QTimer::timeout, this, &Sensore_viewer::slot_on_sub_ch_image);
    image_capture_timer_->setSingleShot(true);
    image_capture_timer_->start(30);

    //connect()

    //image_capture_thread_->start();

    ui->setupUi(this);
}

Sensore_viewer::~Sensore_viewer()
{
    image_capture_timer_->stop();
    /*if (!image_capture_thread_->wait(500)) {
        image_capture_thread_->terminate();
    }*/

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

#include <chrono>
#include <iostream>
using namespace std;
using namespace chrono;


// do something...



void Sensore_viewer::slot_on_sub_ch_image()
{
    int rc, i;
    int count = COUNT_DEFAULT;
    volatile unsigned int addr, size;
    volatile unsigned int *transfer_done;
    int var = 0;
    char *write_allocated = trans.write_buffer;
    addr = 0x1869B0;//0x13780;
    int w = 800;
    int h = 600;
    size = w*h*3;

    static int index = 0;
    if (0)
        fprintf(stdout, "host buffer 0x%x = %p\n",
            size + 4096, trans.write_buffer);

    auto start = system_clock::now();
    if (trans.c2h_fd) {
        rc = read_to_buffer(C2H_DEVICE, trans.c2h_fd, trans.write_buffer, size, addr);
        if (rc < 0) {
            printf("error index %d, read to buffer failed size %d rc %d\n", index, size, rc);
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

    QImage image(reinterpret_cast<uchar *>(trans.write_buffer), w, h, QImage::Format::Format_BGR888);

    image_label_->setPixmap(QPixmap::fromImage(image));

    image_capture_timer_->start(30);
}

