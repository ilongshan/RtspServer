#ifndef RTPCONNECTION_H
#define RTPCONNECTION_H

#include <stdint.h>

#include "Rtp.h"
#include "H264/h264.h"

/**
 * @brief The RtpConnection class
 * 一个RTP连接
 */
class RtpConnection
{
public:
    RtpConnection();

    int is_over_tcp;
    int tcp_sockfd; //if is_over_tcp=1. rtsp socket
    int tcp_interleaved;//if is_over_tcp=1. is rtp interleaved, rtcp interleaved = rtp interleaved + 1
    int udp_sockfd[2]; //if is_over_tcp=0. [0] is rtp socket, [1] is rtcp socket
    uint32_t ssrc;

    static RtpConnection* newRtpConnectionOverTcp(const char *peer_ip, int peer_port_interleaved);
    static RtpConnection* newRtpConnectionOverUdp(const char *peer_ip, int peer_port_interleaved);

    void sendAnNalu(NALU_t *n, int framerate);

private:
    RTP_FIXED_HEADER        *rtp_hdr;

    NALU_HEADER		*nalu_hdr;
    FU_INDICATOR	*fu_ind;
    FU_HEADER		*fu_hdr;

    unsigned int timestamp_increse=0,ts_current=0;
    unsigned short seq_num =0;



    static unsigned long RtpGenSsrc();
    static int RtpRtcpSocket(int *rtpsock, int *rtcpsock, const char *peer_ip, int peer_port);

};

#endif // RTPCLIENT_H
