#include <QSettings>
#include <app_cfg.h>
#include <QApplication>

static app_cfg_info_t g_app_info;


app_cfg_info_t* get_app_cfg_info()
{
    return &g_app_info;
}

void parse_app_cfg_file()
{
    auto path = QApplication::applicationDirPath() + "/cfgs/sensor_viewer_cfg.ini";

    QSettings settings(path, QSettings::IniFormat);

    settings.beginGroup("Image_Sensor");

    g_app_info.width = settings.value("image_width").toInt();
    if (g_app_info.width == 0) g_app_info.width = 1280;
    g_app_info.height = settings.value("image_height").toInt();
    if (g_app_info.height == 0) g_app_info.height = 960;

    qDebug("cfg w %d h %d", g_app_info.width, g_app_info.height);
}
