
SOURCES += \
    $$PWD/src/AppConfig.cpp \
    $$PWD/src/Mutex/Cond.cpp \
    $$PWD/src/Mutex/Mutex.cpp \
    $$PWD/src/Log/log.c \
    $$PWD/src/Thread/CommonThread.cpp \
    $$PWD/src/RTSP/RtspServer.cpp \
    $$PWD/src/RTSP/RtspMsg.cpp \
    $$PWD/src/RTSP/RtspClient.cpp \
    $$PWD/src/RTSP/RtspSession.cpp \
    $$PWD/src/Base64/Base64.cpp \
    $$PWD/src/SDP/RtspSdp.cpp \
    $$PWD/src/RTP/RtpConnection.cpp \
    $$PWD/src/H264/h264.cpp

HEADERS += \
    $$PWD/src/types.h \
    $$PWD/src/AppConfig.h \
    $$PWD/src/Mutex/Cond.h \
    $$PWD/src/Mutex/Mutex.h \
    $$PWD/src/Log/log.h \
    $$PWD/src/Thread/CommonThread.h \
    $$PWD/src/RTSP/RtspServer.h \
    $$PWD/src/RTSP/RtspMsg.h \
    $$PWD/src/RTSP/RtspClient.h \
    $$PWD/src/RTSP/RtspSession.h \
    $$PWD/src/Base64/Base64.h \
    $$PWD/src/SDP/RtspSdp.h \
    $$PWD/src/RTP/RtpConnection.h \
    $$PWD/src/H264/h264.h \
    $$PWD/src/RTP/Rtp.h

INCLUDEPATH += $$PWD/src

