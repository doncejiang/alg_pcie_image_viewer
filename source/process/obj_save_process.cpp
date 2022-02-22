#include "obj_save_process.h"
#include <QFile>

static
int readFile(QString path, int size, uchar* buffer)
{
    int len;
    QFile file(path);

    if (file.open(QIODevice::ReadOnly)) {
        len = file.read((char *)buffer, size);
    } else {
        return -1;
    }
    file.close();
    return len;
}

static
void save_file(QString path, char* buff, int size)
{
    QFile file_(path);

    if (!file_.open(QIODevice::WriteOnly)) {
        return;
    }

    file_.write(buff, size);

    file_.close();
}


object_save_process::object_save_process(QObject *parent) : QObject(parent)
{

}
