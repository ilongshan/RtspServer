#ifndef RTSPSESSION_H
#define RTSPSESSION_H

#include <stdint.h>
#include <string>
#include <list>

#include "Mutex/Cond.h"

#include "H264/h264.h"

class RtspClient;

/**
 * @brief The RtspSession class
 * 一个RTSP会话，也就是一个RTSP流
 */
class RtspSession
{
public:
    RtspSession();

    void setSessionPath(std::string path){mSessionPath = path;}
    std::string getSessionPath(){return mSessionPath;}

    void inputRtspClient(RtspClient* client);
    void removeRtspClient(RtspClient* client);

    void sendAnNalu(NALU_t *n, int framerate);

private:
    std::string mSessionPath; //会话路径

    Cond *mCond;
    std::list<RtspClient*> mRtspClientList; //记录连接到本会话的RTSP客户端

public:
    int  has_video;
    int  has_audio;
    uint8_t h264_sps[64];
    uint8_t h264_pps[64];
    int     h264_sps_len;
    int     h264_pps_len;

    int rtsp_try_set_sps_pps (NALU_t *n);

};

#endif // RTSPSESSION_H
