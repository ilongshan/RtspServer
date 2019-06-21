#include "RtpConnection.h"

#include "types.h"
#include "Log/log.h"

#define H264        96

RtpConnection::RtpConnection()
{

}

void RtpConnection::sendAnNalu(NALU_t *n, int framerate)
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

    char* nalu_payload;
    char sendbuf[1500] = {0};

    int	bytes=0;

    if (framerate <= 0)
    {
        framerate = 15;
    }

    timestamp_increse=(unsigned int)(90000.0 / framerate);

//    printf("start send ...\n");

    memset(sendbuf,0,1500);//���sendbuf����ʱ�Ὣ�ϴε�ʱ�����գ������Ҫts_current�������ϴε�ʱ���ֵ
//rtp�̶���ͷ��Ϊ12�ֽ�,�þ佫sendbuf[0]�ĵ�ַ����rtp_hdr���Ժ��rtp_hdr��д�������ֱ��д��sendbuf��
    rtp_hdr =(RTP_FIXED_HEADER*)&sendbuf[0];
    //����RTP HEADER��
    rtp_hdr->payload     = H264;  //�������ͺţ�
    rtp_hdr->version     = 2;  //�汾�ţ��˰汾�̶�Ϊ2
    rtp_hdr->marker      = 0;   //��־λ���ɾ���Э��涨��ֵ��
    rtp_hdr->ssrc        = htonl(ssrc);    //���ָ��Ϊ10�������ڱ�RTP�Ự��ȫ��Ψһ
//RTSP_LogPrintf("### %d %d\n", n->len, is_over_tcp);
//	��һ��NALUС��1400�ֽڵ�ʱ�򣬲���һ����RTP������
    if(n->len<=1400)
    {
        //����rtp M λ��
        rtp_hdr->marker=1;
        rtp_hdr->seq_no     = htons(seq_num ++); //���кţ�ÿ����һ��RTP����1
        //����NALU HEADER,�������HEADER����sendbuf[12]
        nalu_hdr =(NALU_HEADER*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����nalu_hdr��֮���nalu_hdr��д��ͽ�д��sendbuf�У�
        nalu_hdr->F=n->forbidden_bit;
        nalu_hdr->NRI=n->nal_reference_idc>>5;//��Ч������n->nal_reference_idc�ĵ�6��7λ����Ҫ����5λ���ܽ���ֵ����nalu_hdr->NRI��
        nalu_hdr->TYPE=n->nal_unit_type;

        nalu_payload=&sendbuf[13];//ͬ��sendbuf[13]����nalu_payload
        memcpy(nalu_payload,n->buf+1,n->len-1);//ȥ��naluͷ��naluʣ������д��sendbuf[13]��ʼ���ַ�����

        ts_current=ts_current+timestamp_increse;
        rtp_hdr->timestamp=htonl(ts_current);
        bytes=n->len + 12 ;						//���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ�����NALUͷ����ȥ��ʼǰ׺������rtp_header�Ĺ̶�����12�ֽ�

        uint8_t szbuf[4] = {0};
        if (is_over_tcp)
        {
            int sockfd = tcp_sockfd;
            int size = bytes;
            szbuf[0] = '$';
            szbuf[1] = tcp_interleaved;
            //szbuf[2] = (char)((size & 0xFF00) >> 8);
            //szbuf[3] = (char)(size & 0xFF);
            *((uint16_t*)&szbuf[2]) = htons(size);
            int ret = send(sockfd, (const char*)szbuf, 4, 0);
            if (ret < 0)
            {
                err("rtp send interlaced frame failed: %s\n", strerror(errno));
                return;
            }
        }

        send( socket1, sendbuf, bytes, 0 );//����rtp��
    }
    else if(n->len>1400)
    {
        ///�õ���nalu��Ҫ�ö��ٳ���Ϊ1400�ֽڵ�RTP��������
        int packetNums = n->len/1400;//��Ҫk��1400�ֽڵ�RTP��
        int lastPacketSize = n->len%1400;//���һ��RTP������Ҫװ�ص��ֽ���
        if (lastPacketSize == 0)
        {
            lastPacketSize = 1400;
        }
        else
        {
            packetNums++;
        }

        int currentPacketIndex = 0; //��ǰ���͵İ����

        ts_current=ts_current+timestamp_increse;
        rtp_hdr->timestamp=htonl(ts_current);

        while(currentPacketIndex < packetNums)
        {
            rtp_hdr->seq_no = htons(seq_num ++); //���кţ�ÿ����һ��RTP����1

            if(currentPacketIndex == 0)//����һ����Ҫ��Ƭ��NALU�ĵ�һ����Ƭ����FU HEADER��Sλ
            {
                //����rtp M λ��
                rtp_hdr->marker=0;
                //����FU INDICATOR,�������HEADER����sendbuf[12]
                fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
                fu_ind->F=n->forbidden_bit;
                fu_ind->NRI=n->nal_reference_idc>>5;
                fu_ind->TYPE=28;

                //����FU HEADER,�������HEADER����sendbuf[13]
                fu_hdr =(FU_HEADER*)&sendbuf[13];
                fu_hdr->E=0;
                fu_hdr->R=0;
                fu_hdr->S=1;
                fu_hdr->TYPE=n->nal_unit_type;

                nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]����nalu_payload
                memcpy(nalu_payload,n->buf+1,1400-1);//ȥ��NALUͷ

                bytes=1400-1+14; //���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ���ȥ��ʼǰ׺��NALUͷ������rtp_header��fu_ind��fu_hdr�Ĺ̶�����14�ֽ�

                uint8_t szbuf[4] = {0};
                if (is_over_tcp)
                {
                    int sockfd = tcp_sockfd;
                    int size = bytes;
                    szbuf[0] = '$';
                    szbuf[1] = tcp_interleaved;
                    //szbuf[2] = (char)((size & 0xFF00) >> 8);
                    //szbuf[3] = (char)(size & 0xFF);
                    *((uint16_t*)&szbuf[2]) = htons(size);
                    int ret = send(sockfd, (const char*)szbuf, 4, 0);
                    if (ret < 0)
                    {
                        err("rtp send interlaced frame failed: %s\n", strerror(errno));
                        return;
                    }
                }

                send( socket1, sendbuf, bytes, 0 );//����rtp��

            }
            //����һ����Ҫ��Ƭ��NALU�ķǵ�һ����Ƭ������FU HEADER��Sλ������÷�Ƭ�Ǹ�NALU�����һ����Ƭ����FU HEADER��Eλ
            else if(currentPacketIndex==(packetNums-1))//���͵������һ����Ƭ��ע�����һ����Ƭ�ĳ��ȿ��ܳ���1400�ֽڣ���l>1386ʱ����
            {

                //����rtp M λ����ǰ����������һ����Ƭʱ��λ��1
                rtp_hdr->marker=1;
                //����FU INDICATOR,�������HEADER����sendbuf[12]
                fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
                fu_ind->F=n->forbidden_bit;
                fu_ind->NRI=n->nal_reference_idc>>5;
                fu_ind->TYPE=28;

                //����FU HEADER,�������HEADER����sendbuf[13]
                fu_hdr =(FU_HEADER*)&sendbuf[13];
                fu_hdr->R=0;
                fu_hdr->S=0;
                fu_hdr->TYPE=n->nal_unit_type;
                fu_hdr->E=1;

                nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]�ĵ�ַ����nalu_payload
                memcpy(nalu_payload,n->buf+currentPacketIndex*1400,lastPacketSize);//��nalu���ʣ���l-1(ȥ����һ���ֽڵ�NALUͷ)�ֽ�����д��sendbuf[14]��ʼ���ַ�����
                bytes=lastPacketSize+14;		//���sendbuf�ĳ���,Ϊʣ��nalu�ĳ���l-1����rtp_header��FU_INDICATOR,FU_HEADER������ͷ��14�ֽ�

                uint8_t szbuf[4] = {0};
                if (is_over_tcp)
                {
                    int sockfd = tcp_sockfd;
                    int size = bytes;
                    szbuf[0] = '$';
                    szbuf[1] = tcp_interleaved;
                    //szbuf[2] = (char)((size & 0xFF00) >> 8);
                    //szbuf[3] = (char)(size & 0xFF);
                    *((uint16_t*)&szbuf[2]) = htons(size);
                    int ret = send(sockfd, (const char*)szbuf, 4, 0);
                    if (ret < 0)
                    {
                        err("rtp send interlaced frame failed: %s\n", strerror(errno));
                        return;
                    }
                }

                send( socket1, sendbuf, bytes, 0 );//����rtp��

                break; //���һ����Ƭ�Ļ� ���˳�ѭ�� ��ִ�к����sleep
            }
            else //if(t<k&&0!=t)
            {
                //����rtp M λ��
                rtp_hdr->marker=0;
                //����FU INDICATOR,�������HEADER����sendbuf[12]
                fu_ind =(FU_INDICATOR*)&sendbuf[12]; //��sendbuf[12]�ĵ�ַ����fu_ind��֮���fu_ind��д��ͽ�д��sendbuf�У�
                fu_ind->F=n->forbidden_bit;
                fu_ind->NRI=n->nal_reference_idc>>5;
                fu_ind->TYPE=28;

                //����FU HEADER,�������HEADER����sendbuf[13]
                fu_hdr =(FU_HEADER*)&sendbuf[13];
                //fu_hdr->E=0;
                fu_hdr->R=0;
                fu_hdr->S=0;
                fu_hdr->E=0;
                fu_hdr->TYPE=n->nal_unit_type;

                nalu_payload=&sendbuf[14];//ͬ��sendbuf[14]�ĵ�ַ����nalu_payload
                memcpy(nalu_payload,n->buf+currentPacketIndex*1400,1400);//ȥ����ʼǰ׺��naluʣ������д��sendbuf[14]��ʼ���ַ�����
                bytes=1400+14;						//���sendbuf�ĳ���,Ϊnalu�ĳ��ȣ���ȥԭNALUͷ������rtp_header��fu_ind��fu_hdr�Ĺ̶�����14�ֽ�

                uint8_t szbuf[4] = {0};
                if (is_over_tcp)
                {
                    int sockfd = tcp_sockfd;
                    int size = bytes;
                    szbuf[0] = '$';
                    szbuf[1] = tcp_interleaved;
                    //szbuf[2] = (char)((size & 0xFF00) >> 8);
                    //szbuf[3] = (char)(size & 0xFF);
                    *((uint16_t*)&szbuf[2]) = htons(size);
                    int ret = send(sockfd, (const char*)szbuf, 4, 0);
                    if (ret < 0)
                    {
                        err("rtp send interlaced frame failed: %s\n", strerror(errno));
                        return;
                    }
                }

                send( socket1, sendbuf, bytes, 0 );//����rtp��

            }

            currentPacketIndex++;

            Sleep(2); //����2Ms ���ܷ�̫����
        }
    }

//    printf("send finished\n");

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

