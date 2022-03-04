#ifndef IMAGE_CAPTURE_PROECESS_H
#define IMAGE_CAPTURE_PROECESS_H

#include <QObject>
#include <image_buffer.h>
#include "pcie.h"

class image_capture_proecess : public QObject
{
    Q_OBJECT
public:
    explicit image_capture_proecess(int ch_id, QObject *parent = nullptr);

signals:
    void signal_on_publish_capture_image(void* mete_data, int ch_id);
    void signal_on_publish_init_error_code(int ch_id, int code);

public slots:
    void slot_on_start_sensor_stream();
    void slot_on_recv_pcie_dev_control_port(void* dev_base);

private:
    image_meta_data_t *meta_data_ = nullptr;
    int ch_id_ = 0;
    int frame_index_ = 0;
    pcie_dev* pcie_dev_{nullptr};

};

#endif // IMAGE_CAPTURE_PROECESS_H
