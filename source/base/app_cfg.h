#ifndef APP_CFG_H
#define APP_CFG_H

typedef struct {
    int width = 1280;
    int height = 960;
} app_cfg_info_t;

app_cfg_info_t* get_app_cfg_info();
void parse_app_cfg_file();

#endif // APP_CFG_H
