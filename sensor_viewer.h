#ifndef SENSORE_VIEWER_H
#define SENSORE_VIEWER_H

#include <QMainWindow>
#include <QGridLayout>
#include <QLabel>
#include "dma_utils.h"
#include "image_capture_process.h"
#include "pcie.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Sensore_viewer; }
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



class Sensore_viewer : public QMainWindow
{
    Q_OBJECT

public:
    Sensore_viewer(QWidget *parent = nullptr);
    ~Sensore_viewer();
    int read_buffer2image(uchar* image, int size, uint offset);
    int start_init_camera();
    int get_ch_info();

private:
    QLabel* info_label_;
    Ui::Sensore_viewer *ui;
    QLabel *image_label_[8] = {nullptr};
    QGridLayout* ui_layout_{nullptr};
    QThread* image_capture_thread_{nullptr};
    image_capture_process* image_capture_process_{nullptr};
    uint64_t addr_table[4][7];
    QTimer* image_capture_timer_{nullptr};
    pcie_dev* pcie_dev_;

private slots:
    void slot_on_sub_ch_image();


};
#endif // SENSORE_VIEWER_H
