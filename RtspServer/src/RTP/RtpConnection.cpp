#include "RtpConnection.h"

#include "types.h"
#include "Log/log.h"
#include "AppConfig.h"

#define RTP_HEADER_SIZE 12

#define H264   96
#define AAC    97
#define G711A  97

RtpConnection::RtpConnection()
{

}

unsigned long RtpConnection::RtpGenSsrc()
{
    static unsigned long ssrc = 0x22345678;
    return ssrc++;
}

int RtpConnection::RtpRtcpSocket(int *rtpsock, int *rtcpsock, const char *peer_ip, int peer_port)
{
    int i, ret;

    for (i = 65536/4*3/2*2; i < 65536; i += 2)
    {
        struct sockaddr_in inaddr;
        uint16_t port;

        *rtpsock = socket(AF_INET, SOCK_DGRAM, 0);
        if (*rtpsock < 0)
        {
            err("create rtp socket failed: %s\n", strerror(errno));
            return -1;
        }

        *rtcpsock = socket(AF_INET, SOCK_DGRAM, 0);
        if (*rtcpsock < 0) {
            err("create rtcp socket failed: %s\n", strerror(errno));
            closesocket(*rtpsock);
            return -1;
        }

        port = i;
        memset(&inaddr, 0, sizeof(inaddr));
        inaddr.sin_family = AF_INET;
        inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        inaddr.sin_port = htons(port);
        ret = bind(*rtpsock, (struct sockaddr*)&inaddr, sizeof(inaddr));
        if (ret < 0) {
            closesocket(*rtpsock);
            closesocket(*rtcpsock);
            continue;
        }

        port = i + 1;
        memset(&inaddr, 0, sizeof(inaddr));
        inaddr.sin_family = AF_INET;
        inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        inaddr.sin_port = htons(port);
        ret = bind(*rtcpsock, (struct sockaddr*)&inaddr, sizeof(inaddr));
        if (ret < 0) {
            closesocket(*rtpsock);
            closesocket(*rtcpsock);
            continue;
        }

        port = peer_port / 2 * 2;
        memset(&inaddr, 0, sizeof(inaddr));
        inaddr.sin_family = AF_INET;
        inaddr.sin_addr.s_addr = inet_addr(peer_ip);
        inaddr.sin_port = htons(port);
        ret = connect(*rtpsock, (struct sockaddr*)&inaddr, sizeof(inaddr));
        if (ret < 0)
        {
            closesocket(*rtpsock);
            closesocket(*rtcpsock);
            err("connect peer rtp port failed: %s\n", strerror(errno));
            return -1;
        }

        port = peer_port / 2 * 2 + 1;
        memset(&inaddr, 0, sizeof(inaddr));
        inaddr.sin_family = AF_INET;
        inaddr.sin_addr.s_addr = inet_addr(peer_ip);
        inaddr.sin_port = htons(port);
        ret = connect(*rtcpsock, (struct sockaddr*)&inaddr, sizeof(inaddr));
        if (ret < 0)
        {
            closesocket(*rtpsock);
            closesocket(*rtcpsock);
            err("connect peer rtcp port failed: %s\n", strerror(errno));
            return -1;
        }

        return i;
    }

    err("not found free local port for rtp/rtcp\n");
    return -1;
}

RtpConnection *RtpConnection::newRtpConnectionOverTcp(const char *peer_ip, int peer_port_interleaved)
{
    RtpConnection *rtp = new RtpConnection();

    rtp->is_over_tcp = true;
    rtp->ssrc = RtpGenSsrc();

//    rtp->tcp_sockfd = cc->sockfd;
    rtp->tcp_interleaved = peer_port_interleaved;
//    ret = rtp->tcp_interleaved;

    return rtp;
}

RtpConnection *RtpConnection::newRtpConnectionOverUdp(const char *peer_ip, int peer_port_interleaved)
{
    RtpConnection *rtp = nullptr;

    int rtpsock, rtcpsock;
    int ret = RtpRtcpSocket(&rtpsock, &rtcpsock, peer_ip, peer_port_interleaved);
    if (ret < 0)
    {

    }
    else
    {
        rtp = new RtpConnection();

        rtp->is_over_tcp = false;
        rtp->ssrc = RtpGenSsrc();

        rtp->udp_sockfd[0] = rtpsock;
        rtp->udp_sockfd[1] = rtcpsock;

    }

    return rtp;
}

bool RtpConnection::doSend(const uint8_t *buffer, const int &size)
{
    int socket1;
    if (is_over_tcp)
    {
        socket1 = this->tcp_sockfd;
    }
    else
    {
        socket1 = this->udp_sockfd[0];
    }

    uint8_t szbuf[4] = {0};
    if (is_over_tcp)
    {
        int sockfd = tcp_sockfd;
        szbuf[0] = '$';
        szbuf[1] = tcp_interleaved;
        //szbuf[2] = (char)((size & 0xFF00) >> 8);
        //szbuf[3] = (char)(size & 0xFF);
        *((uint16_t*)&szbuf[2]) = htons(size);
        int ret = send(sockfd, (const char*)szbuf, 4, 0);
        if (ret < 0)
        {
            err("rtp send interlaced frame failed: %s\n", strerror(errno));
            return false;
        }
    }

    int ret = send(socket1, (const char*)buffer, size, 0 );//发送rtp包

    if (ret < 0)
    {
        return false;
    }
    else
    {
        return true;
    }

}

bool RtpConnection::sendH264Buffer(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate)
{
    NALU_t *n = getNALU(frame, len);

    bool isSucceed = false;

    RTP_FIXED_HEADER        *rtp_hdr;

    NALU_HEADER		*nalu_hdr;
    FU_INDICATOR	*fu_ind;
    FU_HEADER		*fu_hdr;

    uint8_t* nalu_payload;
    uint8_t sendbuf[1500] = {0};

    int	bytes=0;

    int socket1;
    if (is_over_tcp)
    {
        socket1 = this->tcp_sockfd;
    }
    else
    {
        socket1 = this->udp_sockfd[0];
    }

    uint32_t rtp_ts = (uint32_t)(timestamp * sample_rate / 1000000);

//    printf("start send ...\n");

//rtp固定包头，为12字节,该句将sendbuf[0]的地址赋给rtp_hdr，以后对rtp_hdr的写入操作将直接写入sendbuf。
    rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0];
    //设置RTP HEADER，
    rtp_hdr->payload     = H264;  //负载类型号，
    rtp_hdr->version     = 2;  //版本号，此版本固定为2
    rtp_hdr->marker      = 0;   //标志位，由具体协议规定其值。
    rtp_hdr->ssrc        = htonl(ssrc);    //需要在本RTP会话中全局唯一
//RTSP_LogPrintf("### %d %d\n", n->len, is_over_tcp);
//	当一个NALU小于1400字节的时候，采用一个单RTP包发送
    if(n->len<=1400)
    {
        //设置rtp M 位；
        rtp_hdr->marker=1;
        rtp_hdr->seq_no     = htons(seq_num ++); //序列号，每发送一个RTP包增1
        //设置NALU HEADER,并将这个HEADER填入sendbuf[12]
        nalu_hdr =(NALU_HEADER*)&sendbuf[12]; //将sendbuf[12]的地址赋给nalu_hdr，之后对nalu_hdr的写入就将写入sendbuf中；
        nalu_hdr->F=n->forbidden_bit;
        nalu_hdr->NRI=n->nal_reference_idc>>5;//有效数据在n->nal_reference_idc的第6，7位，需要右移5位才能将其值赋给nalu_hdr->NRI。
        nalu_hdr->TYPE=n->nal_unit_type;

        nalu_payload=&sendbuf[13];//同理将sendbuf[13]赋给nalu_payload
        memcpy(nalu_payload,n->buf+1,n->len-1);//去掉nalu头的nalu剩余内容写入sendbuf[13]开始的字符串。

        rtp_hdr->timestamp=htonl(rtp_ts);
        bytes=n->len + 12 ;						//获得sendbuf的长度,为nalu的长度（包含NALU头但除去起始前缀）加上rtp_header的固定长度12字节

        isSucceed = doSend(sendbuf, bytes); //发送
    }
    else if(n->len>1400)
    {
        ///得到该nalu需要用多少长度为1400字节的RTP包来发送
        int packetNums = n->len/1400;//需要k个1400字节的RTP包
        int lastPacketSize = n->len%1400;//最后一个RTP包的需要装载的字节数
        if (lastPacketSize == 0)
        {
            lastPacketSize = 1400;
        }
        else
        {
            packetNums++;
        }

        int currentPacketIndex = 0; //当前发送的包序号

        rtp_hdr->timestamp=htonl(rtp_ts);

        while(currentPacketIndex < packetNums)
        {
            rtp_hdr->seq_no = htons(seq_num ++); //序列号，每发送一个RTP包增1

            if(currentPacketIndex == 0)//发送一个需要分片的NALU的第一个分片，置FU HEADER的S位
            {
                //设置rtp M 位；
                rtp_hdr->marker=0;
                //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
                fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
                fu_ind->F=n->forbidden_bit;
                fu_ind->NRI=n->nal_reference_idc>>5;
                fu_ind->TYPE=28;

                //设置FU HEADER,并将这个HEADER填入sendbuf[13]
                fu_hdr =(FU_HEADER*)&sendbuf[13];
                fu_hdr->E=0;
                fu_hdr->R=0;
                fu_hdr->S=1;
                fu_hdr->TYPE=n->nal_unit_type;

                nalu_payload=&sendbuf[14];//同理将sendbuf[14]赋给nalu_payload
                memcpy(nalu_payload,n->buf+1,1400-1);//去掉NALU头

                bytes=1400-1+14; //获得sendbuf的长度,为nalu的长度（除去起始前缀和NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节

                isSucceed = doSend(sendbuf, bytes); //发送
            }
            //发送一个需要分片的NALU的非第一个分片，清零FU HEADER的S位，如果该分片是该NALU的最后一个分片，置FU HEADER的E位
            else if(currentPacketIndex==(packetNums-1))//发送的是最后一个分片，注意最后一个分片的长度可能超过1400字节（当l>1386时）。
            {

                //设置rtp M 位；当前传输的是最后一个分片时该位置1
                rtp_hdr->marker=1;
                //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
                fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
                fu_ind->F=n->forbidden_bit;
                fu_ind->NRI=n->nal_reference_idc>>5;
                fu_ind->TYPE=28;

                //设置FU HEADER,并将这个HEADER填入sendbuf[13]
                fu_hdr =(FU_HEADER*)&sendbuf[13];
                fu_hdr->R=0;
                fu_hdr->S=0;
                fu_hdr->TYPE=n->nal_unit_type;
                fu_hdr->E=1;

                nalu_payload=&sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload
                memcpy(nalu_payload,n->buf+currentPacketIndex*1400,lastPacketSize);//将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。
                bytes=lastPacketSize+14;		//获得sendbuf的长度,为剩余nalu的长度l-1加上rtp_header，FU_INDICATOR,FU_HEADER三个包头共14字节

                isSucceed = doSend(sendbuf, bytes); //发送

                break; //最后一个分片的话 则退出循环 不执行后面的sleep
            }
            else //if(t<k&&0!=t)
            {
                //设置rtp M 位；
                rtp_hdr->marker=0;
                //设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
                fu_ind =(FU_INDICATOR*)&sendbuf[12]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
                fu_ind->F=n->forbidden_bit;
                fu_ind->NRI=n->nal_reference_idc>>5;
                fu_ind->TYPE=28;

                //设置FU HEADER,并将这个HEADER填入sendbuf[13]
                fu_hdr =(FU_HEADER*)&sendbuf[13];
                //fu_hdr->E=0;
                fu_hdr->R=0;
                fu_hdr->S=0;
                fu_hdr->E=0;
                fu_hdr->TYPE=n->nal_unit_type;

                nalu_payload=&sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload
                memcpy(nalu_payload,n->buf+currentPacketIndex*1400,1400);//去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。
                bytes=1400+14;						//获得sendbuf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节

                isSucceed = doSend(sendbuf, bytes); //发送
            }

            currentPacketIndex++;

            AppConfig::mSleep(2); //休眠2Ms 不能发太快了
        }
    }

    FreeNALU(n); //释放NALU内存

    return isSucceed;
}

bool RtpConnection::sendG711ABuffer(const uint8_t *frame, int len, uint64_t timestamp, uint32_t sample_rate)
{
    bool isSucceed = false;

    uint32_t rtp_ts = (uint32_t)(timestamp * sample_rate / 1000000);
    int pos = 0;

    while(1)
    {
        uint8_t sendbuf[1500] = {0};

        int sendLen = len - pos;

        if (sendLen <= 0) break;

        if (sendLen > 1400)
        {
            sendLen = 1400;
        }

    //rtp固定包头，为12字节,该句将sendbuf[0]的地址赋给rtp_hdr，以后对rtp_hdr的写入操作将直接写入sendbuf。
        RTP_FIXED_HEADER* rtp_hdr =(RTP_FIXED_HEADER*)sendbuf; //设置RTP HEADER，
        rtp_hdr->payload    = G711A;  //负载类型号，
        rtp_hdr->version    = 2;  //版本号，此版本固定为2
        rtp_hdr->marker     = 0;   //标志位，由具体协议规定其值。
        rtp_hdr->ssrc       = htonl(ssrc);    //在本RTP会话中全局唯一
        rtp_hdr->seq_no     = htons(seq_num ++); //序列号，每发送一个RTP包增1
        rtp_hdr->timestamp  = htonl(rtp_ts);
        //将要发送的数据填入sendbuf[12]
        memcpy(&sendbuf[RTP_HEADER_SIZE], frame + pos, sendLen);
        pos += sendLen;

        isSucceed = doSend(sendbuf, sendLen + RTP_HEADER_SIZE);

        if (!isSucceed) break;

    }

//    while (len > 0 && count < e->nbpkts)
//    {
//        struct rtphdr *hdr = NULL;
//        int pktsiz = 0;
//        packets[count] = e->szbuf + e->pktsiz * count;
//        hdr = (struct rtphdr*)(packets[count]);
//        pktsiz = e->pktsiz;
//        hdr->v = 2;
//        hdr->p = 0;
//        hdr->x = 0;
//        hdr->cc = 0;
//        hdr->m = (e->seq == 0);
//        hdr->pt = e->pt;
//        hdr->seq = htons(e->seq++);
//        hdr->ts  = htonl(rtp_ts);
//        hdr->ssrc = htonl(e->ssrc);

//        if (len <= pktsiz - RTPHDR_SIZE) {
//            memcpy(packets[count] + RTPHDR_SIZE, frame, len);
//            pktsizs[count] = RTPHDR_SIZE + len;
//            frame += len;
//            len -= len;
//        } else {
//            memcpy(packets[count] + RTPHDR_SIZE, frame, pktsiz - RTPHDR_SIZE);
//            pktsizs[count] = pktsiz;
//            frame += pktsiz - RTPHDR_SIZE;
//            len -= pktsiz - RTPHDR_SIZE;
//        }
//        count ++;
//    }

    return isSucceed;
}
