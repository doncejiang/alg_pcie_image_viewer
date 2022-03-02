#include "sensor_viewer.h"

#include <QApplication>
#include <QFile>
#include <QImage>
#include <QDebug>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Sensore_viewer w;

    w.show();
    return a.exec();
}
