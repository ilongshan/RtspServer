#include "RtspSession.h"

#include "RtspClient.h"
#include "Log/log.h"

RtspSession::RtspSession()
{
    mCond = new Cond;

    this->has_video = true;
    this->has_audio = true;

    h264_sps_len = 0;
    h264_pps_len = 0;

//#define RTP_MAX_PKTSIZ	((1500-42)/4*4)
//#define RTP_MAX_NBPKTS	(300)
//    if (this->has_video) {
//        this->vrtpe.ssrc = 0;
//        this->vrtpe.seq = 0;
//        this->vrtpe.pt = 96;
//        this->vrtpe.sample_rate = 90000;
//        rtp_enc_init(&s->vrtpe, RTP_MAX_PKTSIZ, RTP_MAX_NBPKTS);
//    }
//    if (this>has_audio) {
//        this>artpe.ssrc = 0;
//        this>artpe.seq = 0;
//        this>artpe.pt = 97;
//        this>artpe.sample_rate = 8000;
//        rtp_enc_init(&s->artpe, RTP_MAX_PKTSIZ, 5);
//    }
}

void RtspSession::sendAnNalu(NALU_t *n, int framerate)
{
    rtsp_try_set_sps_pps(n);

    mCond->Lock();
    for (RtspClient* rtspClient : mRtspClientList)
    {
        rtspClient->sendAnNalu(n, framerate);
    }
    mCond->Unlock();
}

void RtspSession::inputRtspClient(RtspClient* client)
{
    mCond->Lock();
    mRtspClientList.push_back(client);
    mCond->Unlock();
}

void RtspSession::removeRtspClient(RtspClient* client)
{
    mCond->Lock();
    if (std::find(mRtspClientList.begin(), mRtspClientList.end(), client) != mRtspClientList.end())
    {
        mRtspClientList.remove(client);
    }
    mCond->Unlock();
}

int RtspSession::rtsp_try_set_sps_pps (NALU_t *n)
{
    uint8_t type = 0;

    int len = n->len;

    if (h264_sps_len > 0 && h264_pps_len > 0)
    {
        return 0;
    }

    type = n->buf[0] & 0x1f;
    len -= n->forbidden_bit;

//    if (frame[0] == 0 && frame[1] == 0 && frame[2] == 1) {
//        type = frame[3] & 0x1f;
//        frame += 3;
//        len   -= 3;
//    }
//    if (frame[0] == 0 && frame[1] == 0 && frame[2] == 0 && frame[3] == 1) {
//        type = frame[4] & 0x1f;
//        frame += 4;
//        len   -= 4;
//    }

    if (len < 1)
        return -1;

    if (type == 7 && 0 == h264_sps_len)
    {
        RTSP_LogPrintf("sps %d\n", len);
        if (len > sizeof(h264_sps))
            len = sizeof(h264_sps);
        memcpy(h264_sps, n->buf, len);
        h264_sps_len = len;
    }

    if (type == 8 && 0 == h264_pps_len)
    {
        RTSP_LogPrintf("pps %d\n", len);
        if (len > sizeof(h264_pps))
            len = sizeof(h264_pps);
        memcpy(h264_pps, n->buf, len);
        h264_pps_len = len;
    }

    return 0;
}
