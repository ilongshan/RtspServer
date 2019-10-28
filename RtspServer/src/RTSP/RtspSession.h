#ifndef RTSPSESSION_H
#define RTSPSESSION_H

#include <stdint.h>
#include <string>
#include <string.h>
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
    RtspSession(bool isHasVideo, bool isHasAudio);

    void setSessionPath(std::string path){mSessionPath = path;}
    std::string getSessionPath(){return mSessionPath;}

    void addRtspClient(RtspClient* client);
    void removeRtspClient(RtspClient* client);

    bool sendH264Buffer(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate);

    bool sendG711A(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate);

    bool sendAACBuffer(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate);

private:
    std::string mSessionPath; //会话路径

    Cond *mCond_RtspClient;
    std::list<RtspClient*> mRtspClientList; //记录连接到本会话的RTSP客户端

public:
    ///音视频数据相关信息
    int  has_video;
    int  has_audio;
    uint8_t h264_sps[64];
    uint8_t h264_pps[64];
    int     h264_sps_len;
    int     h264_pps_len;

    int rtsp_try_set_sps_pps (const uint8_t *h264Buf, int h264BufSize);

};

#endif // RTSPSESSION_H
