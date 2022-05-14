#include "sensor_viewer.h"

#include <QApplication>
#include <QFile>
#include <QImage>
#include <QDebug>
#include <thread>
#include <iostream>

int test_main()
{
    //第一种：使用C提供的time函数计算程序执行的间隔
    time_t start = std::time(NULL);
    std::this_thread::sleep_for(std::chrono::duration<double>(1));

    time_t end = std::time(NULL);

    double elapse = end - start;
    std::cout << "执行的时间为：" << elapse << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    //test_main();
    QApplication a(argc, argv);
    sensor_viewer w;

    w.show();
    return a.exec();
}
