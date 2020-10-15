QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

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
        3rd/CJsonObject.cpp \
        3rd/cJSON.cpp \
        3rd/tinyxml2.cpp \
        bulk3.cpp \
        bulk_common.cpp \
        bulk_config.cpp \
        bulk_database.cpp \
        bulk_net.cpp \
        main.cpp \
        socket.cpp \
        tool.cpp

HEADERS += \
    3rd/CJsonObject.hpp \
    3rd/cJSON.h \
    3rd/tinyxml2.h \
    bulk3.h \
    bulk_common.h \
    bulk_config.h \
    bulk_database.h \
    bulk_net.h \
    darknet.h\
    socket.h \
    yolo_v2_class.hpp

INCLUDEPATH +=/usr/local/include\
                /usr/local/include/opencv\
                /usr/local/include/opencv2\
                /home/wang/downloads/darknet-master/include


LIBS +=/usr/local/lib/libopencv_*.so\
        /home/wang/downloads/darknet-master/libdarknet.so\
        /usr/local/cuda-9.0/lib64/libcudart.so.9.0\
        /usr/local/cuda-9.0/lib64/libcudnn.so.7\
        /usr/local/cuda-9.0/lib64/libcurand.so.9.0\
        /usr/local/cuda-9.0/lib64/libcublas.so.9.0

DISTFILES += \
    bulk3.conf \
    drumDetector-2.pro.user

