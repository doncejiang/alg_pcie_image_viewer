#ifndef IMAGE_CAPTURE_PROCESS_H
#define IMAGE_CAPTURE_PROCESS_H

#include <QObject>
#include "dma_utils.h"

class image_capture_process : public QObject
{
    Q_OBJECT
public:
    explicit image_capture_process(QObject *parent = nullptr);

signals:

};

#endif // IMAGE_CAPTURE_PROCESS_H
