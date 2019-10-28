TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

UI_DIR  = obj/Gui
MOC_DIR = obj/Moc
OBJECTS_DIR = obj/Obj

include ($$PWD/../../RtspServer/RtspServer.pri)

#将输出文件直接放到源码目录下的bin目录下，将dll都放在了此目录中，用以解决运行后找不到dll的问
#DESTDIR=$$PWD/bin/
contains(QT_ARCH, i386) {
    message("32-bit")
    DESTDIR = $${PWD}/bin32
} else {
    message("64-bit")
    DESTDIR = $${PWD}/bin64
}
QMAKE_CXXFLAGS += -std=c++11

SOURCES += \
    src/main.cpp \
    src/H264/h264reader.cpp \
    src/H264/h264decoder.cpp \
    src/AAC/AACReader.cpp

HEADERS += \
    src/H264/h264reader.h \
    src/H264/h264decoder.h \
    src/AAC/AACReader.h

INCLUDEPATH += $$PWD/src

win32{
    LIBS += WS2_32.lib
}

unix{
    LIBS += -lpthread
}
