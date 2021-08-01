#include "sensore_viewer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Sensore_viewer w;
    w.show();
    return a.exec();
}
