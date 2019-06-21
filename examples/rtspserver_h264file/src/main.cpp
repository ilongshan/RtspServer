#include <stdio.h>

#include "RTSP/RtspServer.h"
#include "AppConfig.h"
#include "Log/log.h"

#include "H264/h264reader.h"
#include "H264/h264decoder.h"

//���NALU���Ⱥ�TYPE
void dump(NALU_t *n)
{
    if (!n)return;

    RTSP_LogPrintf(" len: %d nal_unit_type: %x\n", n->len, n->nal_unit_type);
}

int main()
{
    printf("Hello World!\n");

    RtspServer *mRtspServer = new RtspServer();
    mRtspServer->startServer();

    AppConfig::gRtspServer = mRtspServer;

    RtspSession * rtspSession = mRtspServer->newSession("/live/chn1");

    FILE *fp = fopen("../test.h264", "rb");
    if (fp == NULL)
    {
        RTSP_LogPrintf("H264 file not exist!");
        exit(-1);
    }

    int frameNum = 0; //��ǰ���ŵ�֡���
    int frameRate = 0; //֡��

    h264Reader *mH264Reader = new h264Reader();
    uint64_t startTime = AppConfig::getTimeStamp_MilliSecond();

    while(1)
    {
        char buf[10240];
        int size = fread(buf, 1, 1024, fp);//��h264�ļ���1024���ֽ� (ģ��������յ�h264��)

        if (size <= 0) fseek(fp, 0, SEEK_SET);

        int nCount = mH264Reader->inputH264Data((uchar*)buf,size);

        while(1)
        {
            //��ǰ������������л�ȡһ��nalu
            NALU_t* nalu = mH264Reader->getNextFrame();
            if (nalu == NULL) break;

//            dump(nalu);

            ///���ȡ����nalu�Ѿ��޳���ʼ����
            ///������ffmpeg�����������Ҫ������ʼ��
            ///�����Ҫ������һ����ȥ
            int h264BufferSize = nalu->len + nalu->startcodeprefix_len;
            unsigned char * h264Buffer = (unsigned char *)malloc(h264BufferSize);

            if (nalu->startcodeprefix_len == 4)
            {
                h264Buffer[0] = 0x00;
                h264Buffer[1] = 0x00;
                h264Buffer[2] = 0x00;
                h264Buffer[3] = 0x01;
            }
            else
            {
                h264Buffer[0] = 0x00;
                h264Buffer[1] = 0x00;
                h264Buffer[2] = 0x01;
            }

            if (frameRate <= 0)
            {
                uint8_t type = 0;

                int len = nalu->len;

                type = nalu->buf[0] & 0x1f;
                len -= nalu->forbidden_bit;

                if (len < 1)
                    return -1;

                if (type == 7)
                {
                    uint8_t h264_sps[64] = {0};
                    RTSP_LogPrintf("sps %d\n", len);
                    if (len > sizeof(h264_sps))
                        len = sizeof(h264_sps);
                    memcpy(h264_sps, nalu->buf, len);
                    int h264_sps_len = len;

                    int width  = 0;
                    int height = 0;
                    int fps    = 0;

                    bool ret = h264Decoder::decodeSps(h264_sps, h264_sps_len, width, height, fps);

                    RTSP_LogPrintf("ret = %d width = %d height = %d fps = %d\n", ret, width, height, fps);

                    if (fps <= 0) //�����֡��Ϊ0 ˵��sps�в���֡����Ϣ ����������ó�25
                    {
                        fps = 25;
                    }

                    frameRate = fps;
                }

                if (type == 8)
                {
                    RTSP_LogPrintf("pps %d\n", len);
                }

            }

            frameNum++;

            rtspSession->sendAnNalu(nalu, frameRate); //����rtp

            /// h264�����ݲ�����ʱ�����Ϣ  ���ֻ�ܸ���֡����ͬ��
            /// ��Ҫ�ɹ�����һ֡�� ���ܻ�ȡ��֡��
            /// Ϊ0˵����û��ȡ�� ��ֱ����ʾ
            uint64_t currentTime = AppConfig::getTimeStamp_MilliSecond();
            if (frameRate != 0)
            {
                int sleepTime = (1000/frameRate * frameNum) - (currentTime - startTime);

                if (sleepTime>0)
                    Sleep(sleepTime);
            }

            FreeNALU(nalu); //�ͷ�NALU�ڴ�

        }
    }

    return 0;
}
