#include "RtspClient.h"

#include "RtspSession.h"
#include "RtspMsg.h"
#include "Log/log.h"
#include "types.h"

#include "RtspServer.h"
#include "SDP/RtspSdp.h"
#include "AppConfig.h"

#include <string>

const char *__get_peer_ip(int sockfd)
{
    struct sockaddr_in inaddr;
    socklen_t addrlen = sizeof(inaddr);
    int ret = getpeername(sockfd, (struct sockaddr*)&inaddr, &addrlen);
    if (ret < 0) {
        err("getpeername failed: %s\n", strerror(errno));
        return NULL;
    }
    return inet_ntoa(inaddr.sin_addr);
}

RtspClient::RtspClient()
{
    mRtspSession = nullptr;

    vrtp = nullptr;
    artp = nullptr;

    state = RTSP_CC_STATE_INIT;

    memset(mMsgBuffer, 0x0, 4096);
    mMsgLen = 0;

    mCond_RtpSocket = new Cond;

}

RtspClient::~RtspClient()
{
    rtspDelRtpConnection(0);
    rtspDelRtpConnection(1);

    delete mCond_RtpSocket;
    mCond_RtpSocket = nullptr;
}

void RtspClient::sendH264Buffer(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate)
{
    mCond_RtpSocket->Lock();

    if (vrtp != nullptr)
        vrtp->sendH264Buffer(frame, len, timestamp, sample_rate);

    mCond_RtpSocket->Unlock();

}

bool RtspClient::sendG711A(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate)
{
    bool isSucceed = false;

    mCond_RtpSocket->Lock();

    if (artp != nullptr)
        isSucceed = artp->sendG711ABuffer(frame, len, timestamp, sample_rate);

    mCond_RtpSocket->Unlock();

    return isSucceed;
}

int RtspClient::dealwithReceiveBuffer(const char *buffer, const int &bufferLen)
{
    memcpy(mMsgBuffer + mMsgLen, buffer, bufferLen);
    mMsgLen += bufferLen;

    mMsgBuffer[mMsgLen] = '\0';

    rtsp_msg_s msg;
    rtsp_msg_init(&msg);
    int ret = rtsp_msg_parse_from_array(&msg, mMsgBuffer, mMsgLen);
    if (ret < 0)
    {
        RTSP_LogPrintf("Invalid frame\n");
        return -1;
    }

    memmove(mMsgBuffer, mMsgBuffer + ret, mMsgLen - ret);
    mMsgLen -= ret;

    if (msg.type == RTSP_MSG_TYPE_REQUEST)
    {
        rtsp_msg_s responseMsg;
        rtsp_msg_init(&responseMsg);

        ret = processRequest(&msg, &responseMsg);

        if (ret < 0)
        {
            RTSP_LogPrintf("request internal err\n");
        }
        else
        {
            rtspSendMsg(&responseMsg);
        }

        rtsp_msg_free(&responseMsg);
    }
    else if (msg.type == RTSP_MSG_TYPE_INTERLEAVED)
    {
        //TODO process RTCP over TCP frame
        RTSP_LogPrintf("TODO TODO TODO process interleaved frame\n");
    }
    else if (msg.type != RTSP_MSG_TYPE_REQUEST)
    {
        RTSP_LogPrintf("not request frame.\n");
    }

    rtsp_msg_free(&msg);

    return ret;
}

int RtspClient::processRequest(const rtsp_msg_s *requestMsg, rtsp_msg_s *responseMsg)
{
    const char *path = requestMsg->hdrs.startline.reqline.uri.abspath;
    uint32_t cseq = 0, session = 0;

    rtsp_msg_set_response(responseMsg, 200);

    if (mRtspSession == nullptr)
    { ///当前客户端还没有绑定到所属的会话
        bool isPathExist = false;
        for (const RtspSession * rtspSession : AppConfig::gRtspServer->getRtspSessonList())
        {
            const std::string &sessionPath = ((RtspSession*)rtspSession)->getSessionPath();

            if (0 == strncmp(path, sessionPath.c_str(), sessionPath.size()))
            { //XXX rtsp://127.0.0.1/live/chn0000 rtsp://127.0.0.1/live/chn0
                ((RtspSession*)rtspSession)->addRtspClient(this);
                this->mRtspSession = (RtspSession*)rtspSession;
                isPathExist = true;
                break;
            }
        }

        if (!isPathExist)
        {
            RTSP_LogPrintf("[ERROR] Not found session path: %s\n", path);
            rtsp_msg_set_response(responseMsg, 454);
            return 0;
        }

    }
    else
    {
        const std::string &sessionPath = ((RtspSession*)mRtspSession)->getSessionPath();
        if (strncmp(path, sessionPath.c_str(), sessionPath.size()))
        { //XXX rtsp://127.0.0.1/live/chn0000 rtsp://127.0.0.1/live/chn0
            RTSP_LogPrintf("[ERROR] Not allow modify path %s (old:%s)\n", path, sessionPath.c_str());
            rtsp_msg_set_response(responseMsg, 451);
            return 0;
        }
    }

    if (rtsp_msg_get_cseq(requestMsg, &cseq) < 0)
    {
        rtsp_msg_set_response(responseMsg, 400);
        RTSP_LogPrintf("No CSeq field\n");
        return 0;
    }

    if (state != RTSP_CC_STATE_INIT)
    {
        if (rtsp_msg_get_session(requestMsg, &session) < 0 || session != session_id)
        {
            RTSP_LogPrintf("Invalid Session field\n");
            rtsp_msg_set_response(responseMsg, 454);
            return 0;
        }
    }

    rtsp_msg_set_cseq(responseMsg, cseq);
    if (state != RTSP_CC_STATE_INIT)
    {
        rtsp_msg_set_session(responseMsg, session);
    }
    rtsp_msg_set_date(responseMsg, NULL);
    rtsp_msg_set_server(responseMsg, "rtsp_demo");

    switch (requestMsg->hdrs.startline.reqline.method)
    {
        case RTSP_MSG_METHOD_OPTIONS:
            return rtspHandleOPTIONS(requestMsg, responseMsg);
        case RTSP_MSG_METHOD_DESCRIBE:
            return rtspHandleDESCRIBE(requestMsg, responseMsg);
        case RTSP_MSG_METHOD_SETUP:
            return rtspHandleSETUP(requestMsg, responseMsg);
        case RTSP_MSG_METHOD_PAUSE:
            return rtspHandlePAUSE(requestMsg, responseMsg);
        case RTSP_MSG_METHOD_PLAY:
            return rtspHandlePLAY(requestMsg, responseMsg);
        case RTSP_MSG_METHOD_TEARDOWN:
            return rtspHandleTEARDOWN(requestMsg, responseMsg);
        default:
            break;
    }

    rtsp_msg_set_response(responseMsg, 501);

    return 0;
}

int RtspClient::rtspSendMsg(rtsp_msg_s *msg)
{
    char szbuf[1024] = "";
    int ret = rtsp_msg_build_to_array(msg, szbuf, sizeof(szbuf));
    if (ret < 0)
    {
        err("rtsp_msg_build_to_array failed\n");
        return -1;
    }

    ret = send(mSocketFd, szbuf, ret, 0);
    if (ret < 0)
    {
        err("rtsp response send failed: %s\n", strerror(errno));
        return -1;
    }
    return ret;
}

int RtspClient::rtspNewRtpConnection(const char *peer_ip, int peer_port_interleaved, int isaudio, int istcp)
{
    int ret = -1;
    RtpConnection *rtp = nullptr;

    if (istcp)
    {
        rtp = RtpConnection::newRtpConnectionOverTcp(peer_ip, peer_port_interleaved);
        rtp->tcp_sockfd = this->mSocketFd;
        rtp->tcp_interleaved = peer_port_interleaved;
    }
    else
    {
        rtp = RtpConnection::newRtpConnectionOverUdp(peer_ip, peer_port_interleaved);

        if (rtp != nullptr)
        {
            ret = 0;
        }
    }

    if (rtp != nullptr)
    {
        if (isaudio)
            this->artp = rtp;
        else
            this->vrtp = rtp;

        info("rtsp session %08lX new %s %d-%d %s\n",
                this->session_id,
                (isaudio ? "artp" : "vrtp"),
                ret, ret + 1,
                (istcp ? "OverTCP" : "OverUDP"));

        ret = 0;
    }


    return ret;
}

void RtspClient::rtspDelRtpConnection(int isaudio)
{
    RtpConnection *rtp = nullptr;

    if (isaudio)
    {
        rtp = this->artp;
        this->artp = nullptr;
    }
    else
    {
        rtp = this->vrtp;
        this->vrtp = nullptr;
    }

    if (rtp)
    {
        if (!(rtp->is_over_tcp))
        {
            closesocket(rtp->udp_sockfd[0]);
            closesocket(rtp->udp_sockfd[1]);
        }
        delete rtp;
    }
}

int RtspClient::rtspHandleOPTIONS(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg)
{
    uint32_t public_ = 0;
    dbg("\n");
    public_ |= RTSP_MSG_PUBLIC_OPTIONS;
    public_ |= RTSP_MSG_PUBLIC_DESCRIBE;
    public_ |= RTSP_MSG_PUBLIC_SETUP;
    public_ |= RTSP_MSG_PUBLIC_PAUSE;
    public_ |= RTSP_MSG_PUBLIC_PLAY;
    public_ |= RTSP_MSG_PUBLIC_TEARDOWN;
    rtsp_msg_set_public(resmsg, public_);
    return 0;
}

int RtspClient::rtspHandleDESCRIBE(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg)
{
    RtspSession *s = this->getRtspSession();

    char sdp[2048] = {0};
    int len = 0;
    uint32_t accept = 0;
    const rtsp_msg_uri_s *puri = &reqmsg->hdrs.startline.reqline.uri;
    char uri[128] = "";

    dbg("\n");
    if (rtsp_msg_get_accept(reqmsg, &accept) < 0 && !(accept & RTSP_MSG_ACCEPT_SDP)) {
        rtsp_msg_set_response(resmsg, 406);
        warn("client not support accept SDP\n");
        return 0;
    }

    //build uri
    if (puri->scheme == RTSP_MSG_URI_SCHEME_RTSPU)
        strcat(uri, "rtspu://");
    else
        strcat(uri, "rtsp://");
    strcat(uri, puri->ipaddr);
    if (puri->port != 0)
        sprintf(uri + strlen(uri), ":%u", puri->port);

    strcat(uri, s->getSessionPath().c_str());

    len = RtspSdp::buildSimpleSdp(sdp, sizeof(sdp), uri, s->has_video, s->has_audio,
        s->h264_sps, s->h264_sps_len, s->h264_pps, s->h264_pps_len);

    rtsp_msg_set_content_type(resmsg, RTSP_MSG_CONTENT_TYPE_SDP);
    rtsp_msg_set_content_length(resmsg, len);
    resmsg->body.body = rtsp_mem_dup(sdp, len);
    return 0;
}

int RtspClient::rtspHandleSETUP(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg)
{
    RtspSession *s = this->getRtspSession();
    uint32_t ssrc = 0;
    int istcp = 0, isaudio = 0;
    char vpath[64] = "";
    char apath[64] = "";

    dbg("\n");
    if (this->state != RTSP_CC_STATE_INIT && this->state != RTSP_CC_STATE_READY)
    {
        rtsp_msg_set_response(resmsg, 455);
        warn("rtsp status err\n");
        return 0;
    }

    if (!reqmsg->hdrs.transport)
    {
        rtsp_msg_set_response(resmsg, 461);
        warn("rtsp no transport err\n");
        return 0;
    }

    if (reqmsg->hdrs.transport->type == RTSP_MSG_TRANSPORT_TYPE_RTP_AVP_TCP)
    {
        istcp = 1;
        if (!(reqmsg->hdrs.transport->flags & RTSP_MSG_TRANSPORT_FLAG_INTERLEAVED))
        {
            warn("rtsp no interleaved err\n");
            rtsp_msg_set_response(resmsg, 461);
            return 0;
        }
    }
    else
    {
        if (!(reqmsg->hdrs.transport->flags & RTSP_MSG_TRANSPORT_FLAG_CLIENT_PORT))
        {
            warn("rtsp no client_port err\n");
            rtsp_msg_set_response(resmsg, 461);
            return 0;
        }
    }

    snprintf(vpath, sizeof(vpath) - 1, "%s/track1", s->getSessionPath().c_str());
    snprintf(apath, sizeof(vpath) - 1, "%s/track2", s->getSessionPath().c_str());
    if (s->has_video && 0 == strncmp(reqmsg->hdrs.startline.reqline.uri.abspath, vpath, strlen(vpath)))
    {
        isaudio = 0;
    }
    else if (s->has_audio && 0 == strncmp(reqmsg->hdrs.startline.reqline.uri.abspath, apath, strlen(apath)))
    {
        isaudio = 1;
    }
    else
    {
        warn("rtsp urlpath:%s err\n vpath=%s \n apath=%s \n", reqmsg->hdrs.startline.reqline.uri.abspath, vpath, apath);
        rtsp_msg_set_response(resmsg, 461);
        return 0;
    }

    rtspDelRtpConnection(isaudio);

    RTSP_LogPrintf("\n\n\n\n\n\n\n============================\n%s istcp=%d \n========================================\n\n\n\n\n\n\n", __FUNCTION__, istcp);

    if (istcp)
    {
        int ret = rtspNewRtpConnection(__get_peer_ip(this->mSocketFd), reqmsg->hdrs.transport->interleaved, isaudio, 1);

        if (ret < 0)
        {
            rtsp_msg_set_response(resmsg, 500);
            return 0;
        }
        ssrc = isaudio ? this->artp->ssrc : this->vrtp->ssrc;
        rtsp_msg_set_transport_tcp(resmsg, ssrc, reqmsg->hdrs.transport->interleaved);
    }
    else
    {
        int ret = rtspNewRtpConnection(__get_peer_ip(this->mSocketFd), reqmsg->hdrs.transport->client_port, isaudio, 0);
        if (ret < 0)
        {
            rtsp_msg_set_response(resmsg, 500);
            return 0;
        }
        ssrc = isaudio ? this->artp->ssrc : this->vrtp->ssrc;
        rtsp_msg_set_transport_udp(resmsg, ssrc, reqmsg->hdrs.transport->client_port, ret);
    }

    if (this->state == RTSP_CC_STATE_INIT)
    {
        this->state = RTSP_CC_STATE_READY;
        this->session_id = rtsp_msg_gen_session_id();
        rtsp_msg_set_session(resmsg, this->session_id);
    }

    return 0;
}

int RtspClient::rtspHandlePAUSE(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg)
{
    RtspClient * cc = this;

    dbg("\n");
    if (cc->state != RTSP_CC_STATE_READY && cc->state != RTSP_CC_STATE_PLAYING)
    {
        rtsp_msg_set_response(resmsg, 455);
        warn("rtsp status err\n");
        return 0;
    }

    if (cc->state != RTSP_CC_STATE_READY)
        cc->state = RTSP_CC_STATE_READY;
    return 0;
}

int RtspClient::rtspHandlePLAY(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg)
{
    RtspClient *cc = this;

    dbg("\n");
    if (cc->state != RTSP_CC_STATE_READY && cc->state != RTSP_CC_STATE_PLAYING)
    {
        rtsp_msg_set_response(resmsg, 455);
        warn("rtsp status err\n");
        return 0;
    }

    if (cc->state != RTSP_CC_STATE_PLAYING)
        cc->state = RTSP_CC_STATE_PLAYING;
    return 0;
}

int RtspClient::rtspHandleTEARDOWN(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg)
{
    RtspClient *cc = this;

    dbg("\n");
    rtspDelRtpConnection(0);
    rtspDelRtpConnection(1);
    cc->state = RTSP_CC_STATE_INIT;
    return 0;
}

