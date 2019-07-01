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

    /**
     * @brief sendH264Buffer
     * @param frame
     * @param len
     * @param timestamp   时间戳，相对于1,1000 的时间戳
     * @param sample_rate 采样率 H264设置为90000， 音频为实际采样率比如：44100
     * @return
     */
    bool sendH264Buffer(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate);

    /**
     * @brief sendG711A
     * @param frame
     * @param len
     * @param timestamp   时间戳，相对于1,1000 的时间戳
     * @param sample_rate 采样率 H264设置为90000， 音频为实际采样率比如：44100
     * @return
     */
    bool sendG711ABuffer(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate);

private:
    unsigned short seq_num =0;

    bool doSend(const uint8_t *buffer, const int &size);

    static unsigned long RtpGenSsrc();
    static int RtpRtcpSocket(int *rtpsock, int *rtcpsock, const char *peer_ip, int peer_port);

};

#endif // RTPCLIENT_H
