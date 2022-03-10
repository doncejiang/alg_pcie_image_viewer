#ifndef APP_CONFIG_H
#define APP_CONFIG_H


#define MAX_DEV_CH_NUM 8

#define DEF_BUFFER_DEPTH   (10u)
#define MAX_IMAGE_WIDTH    (3840u)
#define MAX_IMAGE_HEIGHT   (2160u)
#define MAX_PIXEL_BYTE_NUM (2u)

typedef enum {
    STATUS_OK,
    STATUS_ERROR,

} alg_error_status_t;



#endif // APP_CONFIG_H
