#ifndef RTSPCLIENT_H
#define RTSPCLIENT_H

#include "RtspMsg.h"
#include "RTP/RtpConnection.h"

class RtspSession;

/**
 * @brief The RtspClient class
 * һ�����ӽ�����RTSP �ͻ���
 */
class RtspClient
{
public:
    RtspClient();
    ~RtspClient();

    void setSocketFd(int fd){mSocketFd = fd;}
    int getSocketFd(){return mSocketFd;}

    ///�����յ�����Ϣ
    int dealwithReceiveBuffer(const char *buffer, const int &bufferLen);

    RtspSession *getRtspSession(){return mRtspSession;}

    void sendAnNalu(NALU_t *n, int framerate);

private:
    int mSocketFd;  //rtsp client socket

    char mMsgBuffer[4096];
    int mMsgLen;

    RtspSession *mRtspSession; //��¼���ͻ���������RTSP�Ự

    int processRequest(const rtsp_msg_s *requestMsg, rtsp_msg_s *responseMsg);

    int rtspSendMsg(rtsp_msg_s *msg);

public:
#define RTSP_CC_STATE_INIT		0
#define RTSP_CC_STATE_READY		1
#define RTSP_CC_STATE_PLAYING	2
#define RTSP_CC_STATE_RECORDING	3

    int state;	//session state
    unsigned long session_id;	//session id

    RtpConnection *vrtp;
    RtpConnection *artp;

    ///����ͻ��˷�����������
    int rtspHandleOPTIONS(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg);
    int rtspHandleDESCRIBE(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg);
    int rtspHandleSETUP(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg);
    int rtspHandlePAUSE(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg);
    int rtspHandlePLAY(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg);
    int rtspHandleTEARDOWN(const rtsp_msg_s *reqmsg, rtsp_msg_s *resmsg);

    ///�����µ�RTP����
    int rtspNewRtpConnection(const char *peer_ip, int peer_port_interleaved, int isaudio, int istcp);

    ///ɾ��RTP����
    void rtspDelRtpConnection(int isaudio);

};

#endif // RTSPCLIENT_H
