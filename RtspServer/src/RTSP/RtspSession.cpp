#include "RtspSession.h"

#include "RtspClient.h"
#include "Log/log.h"

RtspSession::RtspSession(bool isHasVideo, bool isHasAudio)
{
    mCond_RtspClient = new Cond;

    this->has_video = isHasVideo;
    this->has_audio = isHasAudio;

    h264_sps_len = 0;
    h264_pps_len = 0;
}

bool RtspSession::sendH264Buffer(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate)
{
    rtsp_try_set_sps_pps(frame, len);

    mCond_RtspClient->Lock();
    for (RtspClient* rtspClient : mRtspClientList)
    {
        rtspClient->sendH264Buffer(frame, len, timestamp, sample_rate);
    }
    mCond_RtspClient->Unlock();

    return true;
}

bool RtspSession::sendG711A(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate)
{
    mCond_RtspClient->Lock();
    for (RtspClient* rtspClient : mRtspClientList)
    {
        rtspClient->sendG711A(frame, len, timestamp, sample_rate);
    }
    mCond_RtspClient->Unlock();

    return true;
}

void RtspSession::addRtspClient(RtspClient* client)
{
    mCond_RtspClient->Lock();
    mRtspClientList.push_back(client);
    mCond_RtspClient->Unlock();
}

void RtspSession::removeRtspClient(RtspClient* client)
{
    mCond_RtspClient->Lock();
    if (std::find(mRtspClientList.begin(), mRtspClientList.end(), client) != mRtspClientList.end())
    {
        mRtspClientList.remove(client);
    }
    mCond_RtspClient->Unlock();
}

int RtspSession::rtsp_try_set_sps_pps (const uint8_t *h264Buf, int h264BufSize)
{
    uint8_t type = 0;

    if (h264_sps_len > 0 && h264_pps_len > 0)
    {
        return 0;
    }

    int StartCode = 0;

    uint8_t *Buf = (uint8_t *)h264Buf;

    if(Buf[0]==0 && Buf[1]==0 && Buf[2] ==0 && Buf[3] ==1)
     //Check whether buf is 0x00000001
    {
        StartCode = 4;
    }
    else if(Buf[0]==0 && Buf[1]==0 && Buf[2] ==1)
    { //Check whether buf is 0x000001
        StartCode = 3;
    }
    else
    {
        //否则 往后查找一个字节
        return 0;
    }

    Buf += StartCode;

    type = Buf[0] & 0x1f;
    int len = h264BufSize - StartCode;

    if (len < 1)
        return -1;

    if (type == 7 && 0 == h264_sps_len)
    {
        RTSP_LogPrintf("sps %d\n", len);
        if (len > sizeof(h264_sps))
            len = sizeof(h264_sps);
        memcpy(h264_sps, Buf, len);
        h264_sps_len = len;
    }

    if (type == 8 && 0 == h264_pps_len)
    {
        RTSP_LogPrintf("pps %d\n", len);
        if (len > sizeof(h264_pps))
            len = sizeof(h264_pps);
        memcpy(h264_pps, Buf, len);
        h264_pps_len = len;
    }

    return 0;
}
