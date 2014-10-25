TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    raspicvcam.cpp \
    raspicamcv/tga.c \
    raspicamcv/RaspiVid.c \
    raspicamcv/RaspiTexUtil.c \
    raspicamcv/RaspiTex.c \
    raspicamcv/RaspiStillYUV.c \
    raspicamcv/RaspiPreview.c \
    raspicamcv/RaspiCLI.c \
    raspicamcv/RaspiCamControl.c \
    raspicamcv/gl_scenes/yuv.c \
    raspicamcv/gl_scenes/teapot.c \
    raspicamcv/gl_scenes/square.c \
    raspicamcv/gl_scenes/sobel.c \
    raspicamcv/gl_scenes/models.c \
    raspicamcv/gl_scenes/mirror.c \
    serialib/serialib.cpp \
    UserProtocol.cpp

HEADERS += \
    raspicvcam.h \
    raspicamcv/tga.h \
    raspicamcv/RaspiTexUtil.h \
    raspicamcv/RaspiTex.h \
    raspicamcv/RaspiPreview.h \
    raspicamcv/RaspiCLI.h \
    raspicamcv/RaspiCamControl.h \
    raspicamcv/gl_scenes/yuv.h \
    raspicamcv/gl_scenes/teapot.h \
    raspicamcv/gl_scenes/square.h \
    raspicamcv/gl_scenes/sobel.h \
    raspicamcv/gl_scenes/models.h \
    raspicamcv/gl_scenes/mirror.h \
    raspicamcv/gl_scenes/cube_texture_and_coords.h \
    serialib/serialib.h \
    UserProtocol.h

OTHER_FILES += \
    raspicamcv/RaspiCamDocs.odt

INCLUDEPATH += /opt/nsysroot/opt/vc/userland/host_applications/linux/libs/bcm_host/include \
                /opt/nsysroot/opt/vc/userland/interface/vcos \
                /opt/nsysroot/opt/vc/userland \
                /opt/nsysroot/opt/vc/userland/interface/vcos/pthreads \
                /opt/nsysroot/opt/vc/userland/interface/vmcs_host/linux \
                /opt/nsysroot/opt/vc/userland/interface/khronos/include \
                /opt/nsysroot/opt/vc/userland/interface/khronos/common \
                /opt/nsysroot/opt/vc/include \
                /opt/nsysroot/opt/vc/include/interface/vcos \
                /opt/nsysroot/opt/vc/include/interface/vcos/pthreads \
                /opt/nsysroot/opt/vc/include/interface/vmcs_host/linux

LIBS += -L/opt/nsysroot/opt/vc/lib \
                -lmmal_core \
                -lmmal_util \
                -lmmal_vc_client \
                -lvcos \
                -lbcm_host \
                -lGLESv2 \
                -lEGL

LIBS += -L/opt/nsysroot/usr/lib/arm-linux-gnueabihf \
                -lpthread

INCLUDEPATH += /opt/nsysroot/usr/local/include

INCLUDEPATH += $$PWD/opencv/include
LIBS += -L$$PWD/opencv/lib \
                -lopencv_calib3d \
                -lopencv_contrib \
                -lopencv_core \
                -lopencv_features2d \
                -lopencv_flann \
                -lopencv_highgui \
                -lopencv_imgproc \
                -lopencv_legacy \
                -lopencv_ml \
                -lopencv_objdetect \
                -lopencv_ts \
                -lopencv_video
