#include "sensore_viewer.h"

#include <QApplication>
#include <QFile>
#include <QImage>
#include <QDebug>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Sensore_viewer w;

    /*alg_cv::cvtColor(buffer_raw, buffer_rgb, 1280, 960, alg_cv::YUV422_YUYV_2_RGB);

    QImage image((const uchar*)buffer_rgb, 1280, 960, QImage::Format_RGB888);
    image = image.scaled(640, 480);

    QLabel *label = new QLabel;
    label->setPixmap(QPixmap::fromImage(image));
    label->show();*/


    w.show();
    return a.exec();
}
