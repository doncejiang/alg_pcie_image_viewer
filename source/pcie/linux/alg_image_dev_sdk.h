#include "pcie_dev_utils.h"

#define MAX_TRANS_LEN 128

enum ALG_ERR_E {

};

typedef  struct frm_info {
    uint32_t frm_index;
    uint16_t width;
    uint16_t height;
    uint8_t   data_type;
    uint8_t   plane_info;
    uint8_t    vc_id;
    uint8_t    rsvd;
    float        again;
    float        dgain;
    float        temp;
    const void*  img_ptr;
    size_t     img_size;
} frm_info_t;


typedef  struct can_msg {
    uint32_t frm_index;
    uint16_t can_id;
    uint16_t  remote_id;
    uint8_t   msg_type; //ext or std frame
    char       msg_ptr[MAX_TRANS_LEN];
    size_t     msg_size;
} can_msg_t;


typedef  struct uart_msg {
    uint32_t frm_index;
    uint16_t can_id;
    uint16_t  remote_id;
    uint8_t   msg_type; //ext or std frame
    char     msg_ptr[MAX_TRANS_LEN];
    size_t     msg_size;
} uart_msg_t;


typedef  struct iic_msg {
    uint32_t frm_index;
    uint8_t iic_id;
    uint16_t  reg;
    uint8_t   msg_type; //ext or std frame
    char     msg_ptr[MAX_TRANS_LEN];
    size_t     msg_size;
} iic_msg_t;


ALG_ERR_E alg_card_dev_open(int  index,  const char* serial_number);
ALG_ERR_E alg_card_dev_close(int  index);
ALG_ERR_E alg_card_dev_recv_img(frm_info_t* frm_info,  uint timeout, int channel);
ALG_ERR_E alg_card_dev_iic_read(iic_msg_t* frm_info,  uint timeout, int channel);
ALG_ERR_E alg_card_dev_iic_write(iic_msg_t* frm_info,  uint timeout, int channel);
ALG_ERR_E alg_card_dev_can_write(can_msg_t* msg);
ALG_ERR_E alg_card_dev_uart_write(uart_msg_t* msg);
ALG_ERR_E alg_card_dev_can_read(can_msg_t* msg, uint timeout);
ALG_ERR_E alg_card_dev_uart_read(uart_msg_t* msg, uint timeout);