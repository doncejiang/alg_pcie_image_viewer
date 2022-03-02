QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ./source/process/image_capture_process.cpp \
    main.cpp \
    sensore_viewer.cpp \
    source/algroithm/alg_cvtColor.cpp \
    source/base/app_cfg.cpp \
    source/base/image_buffer.cpp \
    source/pcie/dma_utils.c \
    source/pcie/pcie_protocol.cpp \
    source/process/obj_save_process.cpp

HEADERS += \
    ./source/process/image_capture_process.h \
    sensore_viewer.h \
    source/algroithm/alg_cvtColor.h \
    source/base/app_cfg.h \
    source/base/app_config.h \
    source/base/image_buffer.h \
    source/pcie/dma_utils.h \
    source/pcie/pcie_protocol.h \
    source/pcie/protocol_mem_cfg.h \
    source/process/obj_save_process.h

FORMS += \
    sensore_viewer.ui

TRANSLATIONS += \
    sensor_viewer_vcard_zh_CN.ts

INCLUDEPATH += \
    ./source/process/ \
    ./source/pcie/ \
    ./source/algroithm \
    ./source/base

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
