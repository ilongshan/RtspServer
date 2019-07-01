#include <stdio.h>

#include "RTSP/RtspServer.h"
#include "AppConfig.h"
#include "Log/log.h"

#include "H264/h264reader.h"
#include "H264/h264decoder.h"

#include <thread>

uint64_t startTime = AppConfig::getTimeStamp_MilliSecond();

//输出NALU长度和TYPE
void dump(NALU_t *n)
{
    if (!n)return;

    RTSP_LogPrintf(" len: %d nal_unit_type: %x\n", n->len, n->nal_unit_type);
}
 
void readH264File(RtspSession * rtspSession)
{
    FILE *fp = fopen("../BarbieGirl.h264", "rb");
    if (fp == NULL)
    {
        RTSP_LogPrintf("H264 file not exist!");
        exit(-1);
    }

    int frameNum = 0; //当前播放的帧序号
    int frameRate = 0; //帧率

    uint64_t ts = 0;

    h264Reader *mH264Reader = new h264Reader();

    while(1)
    {
        char buf[10240] = {0};
        int size = fread(buf, 1, 1024, fp);//从h264文件读1024个字节 (模拟从网络收到h264流)

        if (size <= 0) fseek(fp, 0, SEEK_SET);

        int nCount = mH264Reader->inputH264Data((uchar*)buf,size);

        while(1)
        {
            uint8_t *h264Buf = nullptr;
            int h264BufSize = 0;

            //从前面读到的数据中获取一个nalu
            NALU_t* nalu = mH264Reader->getNextFrame(h264Buf, h264BufSize);
            if (nalu == NULL) break;

//            dump(nalu);

            if (frameRate <= 0)
            {
                uint8_t type = 0;

                int len = nalu->len;

                type = nalu->buf[0] & 0x1f;
                len -= nalu->forbidden_bit;

                if (len < 1)
                    return;

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

                    ///解析SPS信息，从中获取视频宽高和帧率
                    bool ret = h264Decoder::decodeSps(h264_sps, h264_sps_len, width, height, fps);

                    RTSP_LogPrintf("ret = %d width = %d height = %d fps = %d\n", ret, width, height, fps);

                    if (fps <= 0) //解析后帧率为0 说明sps中不带帧率信息 这里给他设置成25
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

#define AUDIO_SAMPLE_RATE 90000

            rtspSession->sendH264Buffer(h264Buf, h264BufSize, ts, AUDIO_SAMPLE_RATE);

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

            ts = (currentTime - startTime) * 1000; //微秒

            FreeNALU(nalu); //释放NALU内存

            free(h264Buf);
        }
    }
}

void readG711AFile(RtspSession * rtspSession)
{
    FILE *fp = fopen("../BarbieGirl.alaw", "rb");
    if (fp == NULL)
    {
        RTSP_LogPrintf("G711A file not exist!");
        exit(-1);
    }

    uint64_t ts = 0;


#define AUDIO_FRAME_SIZE 320
#define AUDIO_SAMPLE_RATE 8000

    while(1)
    {
        char buf[10240] = {0};
        int size = fread(buf, 1, AUDIO_FRAME_SIZE, fp);//从h264文件读1024个字节 (模拟从网络收到h264流)

        if (size <= 0) fseek(fp, 0, SEEK_SET);

        uint64_t currentTime = AppConfig::getTimeStamp_MilliSecond();

        ts = (currentTime - startTime) * 1000;

        Sleep(1000 / 25);

        rtspSession->sendG711A((uint8_t*)buf, AUDIO_FRAME_SIZE, ts, AUDIO_SAMPLE_RATE);

    }
}

int main()
{
    printf("Hello World!\n");

    RtspServer *mRtspServer = new RtspServer();
    mRtspServer->startServer();

    AppConfig::gRtspServer = mRtspServer;

    RtspSession * rtspSession = mRtspServer->newSession("/live/chn1", false, true);

    startTime = AppConfig::getTimeStamp_MilliSecond();

    //启动新的线程读取视频文件
    std::thread([&](RtspSession * rtspSession)
    {
        readH264File(rtspSession);

    }, rtspSession).detach();

    //启动新的线程读取音频文件
    std::thread([&](RtspSession * rtspSession)
    {
        readG711AFile(rtspSession);

    }, rtspSession).detach();

    while(1) Sleep(1000);

    return 0;
}
