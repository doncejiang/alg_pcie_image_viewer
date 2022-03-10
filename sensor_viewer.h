#ifndef sensor_viewer_H
#define sensor_viewer_H

#include <QMainWindow>
#include <QGridLayout>
#include <QLabel>
#include "image_capture_process.h"
#include "pcie.h"

QT_BEGIN_NAMESPACE
namespace Ui { class sensor_viewer; }
QT_END_NAMESPACE



#define H2C_DEVICE "/dev/xdma0_h2c_0"
#define C2H_DEVICE "/dev/xdma0_c2h_0"
#define REG_DEVICE_NAME "/dev/xdma0_xvc"
#define OUTPUT_FILENAME "out.ts"
#define COUNT_DEFAULT 		  (1)
#define SIZE_DEFAULT		  (32)


/* Input file params */
#define INPUT_WIDTH_DEFAULT        (1920)
#define INPUT_HEIGHT_DEFAULT       (1080)



class sensor_viewer : public QMainWindow
{
    Q_OBJECT

public:
    sensor_viewer(QWidget *parent = nullptr);
    ~sensor_viewer();
    int read_buffer2image(uchar* image, int size, uint offset);
    int start_init_camera();

private:
    QLabel* info_label_;
    Ui::sensor_viewer *ui;
    QLabel *image_label_[8] = {nullptr};
    QLabel* ch_info_label[8] = {nullptr};
    QGridLayout* ui_layout_{nullptr};
    QThread* image_capture_thread_[8]{nullptr};
    image_capture_proecess* image_capture_process_[8]{nullptr};
    //uint64_t addr_table[4][7];
    QTimer* image_capture_timer_{nullptr};
    pcie_dev* pcie_dev_{nullptr};

    char* image_chache{nullptr};
    char* image_chache_rgb{nullptr};

private slots:
    void slot_on_sub_ch_image();
    void slot_on_recv_ch_meta_data(void* meta, int ch);

signals:
    void signal_on_pub_dev_instance(void* pcie_dev);
    void signal_on_start_sensor_stream();


};
#endif // sensor_viewer_H
