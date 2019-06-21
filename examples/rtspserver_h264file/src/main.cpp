#include <stdio.h>

#include "RTSP/RtspServer.h"
#include "AppConfig.h"
#include "Log/log.h"

#include "H264/h264reader.h"
#include "H264/h264decoder.h"

//输出NALU长度和TYPE
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

    int frameNum = 0; //当前播放的帧序号
    int frameRate = 0; //帧率

    h264Reader *mH264Reader = new h264Reader();
    uint64_t startTime = AppConfig::getTimeStamp_MilliSecond();

    while(1)
    {
        char buf[10240];
        int size = fread(buf, 1, 1024, fp);//从h264文件读1024个字节 (模拟从网络收到h264流)

        if (size <= 0) fseek(fp, 0, SEEK_SET);

        int nCount = mH264Reader->inputH264Data((uchar*)buf,size);

        while(1)
        {
            //从前面读到的数据中获取一个nalu
            NALU_t* nalu = mH264Reader->getNextFrame();
            if (nalu == NULL) break;

//            dump(nalu);

            ///这读取到的nalu已经剔除起始码了
            ///而传给ffmpeg解码的数据需要带上起始码
            ///因此需要给他加一个上去
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

                    if (fps <= 0) //解码后帧率为0 说明sps中不带帧率信息 这里给他设置成25
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

            rtspSession->sendAnNalu(nalu, frameRate); //发送rtp

            /// h264裸数据不包含时间戳信息  因此只能根据帧率做同步
            /// 需要成功解码一帧后 才能获取到帧率
            /// 为0说明还没获取到 则直接显示
            uint64_t currentTime = AppConfig::getTimeStamp_MilliSecond();
            if (frameRate != 0)
            {
                int sleepTime = (1000/frameRate * frameNum) - (currentTime - startTime);

                if (sleepTime>0)
                    Sleep(sleepTime);
            }

            FreeNALU(nalu); //释放NALU内存

        }
    }

    return 0;
}
