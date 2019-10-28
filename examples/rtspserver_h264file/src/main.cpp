#include <stdio.h>

#include "RTSP/RtspServer.h"
#include "AppConfig.h"
#include "Log/log.h"

#include "H264/h264reader.h"
#include "H264/h264decoder.h"

#include "AAC/AACReader.h"

#include <thread>

#define VIDEO_SAMPLE_RATE 90000
#define AUDIO_FRAME_SIZE 320
#define AUDIO_SAMPLE_RATE 8000

uint64_t audioPts = 0;

//输出NALU长度和TYPE
void dump(NALU_t *n)
{
    if (!n)return;

    RTSP_LogPrintf(" len: %d nal_unit_type: %x\n", n->len, n->nal_unit_type);
}

void readH264File(RtspSession * rtspSession)
{
    FILE *fp = fopen("../BarbieGirl.h264", "rb");
//    FILE *fp = fopen("E:/out.h264", "rb");
    if (fp == NULL)
    {
        RTSP_LogPrintf("H264 file not exist!");
        exit(-1);
    }

    int frameNum = 0; //当前播放的帧序号
    int frameRate = 0; //帧率

    h264Reader *mH264Reader = new h264Reader();

    while(1)
    {
        char buf[10240] = {0};
        int size = fread(buf, 1, 1024, fp);//从h264文件读1024个字节 (模拟从网络收到h264流)

        if (size <= 0)
        {
            RTSP_LogPrintf("readH264File finished! \n");

            fseek(fp, 0, SEEK_SET);
            Sleep(1000);
            continue;
        }

        int nCount = mH264Reader->inputH264Data((uchar*)buf,size);

        while(1)
        {
            uint8_t *h264Buf = nullptr;
            int h264BufSize = 0;

            //从前面读到的数据中获取一个nalu
            NALU_t* nalu = mH264Reader->getNextFrame(h264Buf, h264BufSize);
            if (nalu == NULL) break;

//            dump(nalu);

            ///计算帧率
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

            ///统计帧数
            {
                uint8_t type = 0;

                int len = nalu->len;

                type = nalu->buf[0] & 0x1f;
                len -= nalu->forbidden_bit;

                ///sps不计入帧数统计，不用于计算pts
                if (type == 7)
                {
//                    RTSP_LogPrintf("is sps len=%d\n", len);
                }
                else if (type == 8)
                {
//                    RTSP_LogPrintf("is pps len=%d\n", len);
                    frameNum++;
                }
                else
                {
                    frameNum++;
                }
            }

            /// h264裸数据不包含时间戳信息  因此只能根据帧率做同步
            /// 需要成功解码一帧后 才能获取到帧率
            uint64_t videoPts = (1000000 / frameRate * frameNum); //微秒

            ///根据音频时间戳做同步。
            while (videoPts > audioPts || audioPts <= 0)
            {
                Sleep(5);
            }

            rtspSession->sendH264Buffer(h264Buf, h264BufSize, videoPts, VIDEO_SAMPLE_RATE);

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


//    G711的打包周期分为10ms,20ms,30ms,sample rate是8000，速率是64kbit/s
//    64kbits,意味着每秒发送64000比特
//    那么10ms发送= 64000 * (10/1000) = 640 比特 = 80 字节
//    那么10ms的包 = 80字节
//    20ms = 160 字节
//    30ms = 240 字节
//    40oms = 320 字节
//    这里每40ms发送一次数据（320字节）

    while(1)
    {
        char buf[10240] = {0};
        int size = fread(buf, 1, AUDIO_FRAME_SIZE, fp);//从h264文件读1024个字节 (模拟从网络收到h264流)

        if (size <= 0)
        {
            RTSP_LogPrintf("readG711AFile finished! \n");

            fseek(fp, 0, SEEK_SET);
            Sleep(1000);
            continue;
        }

        audioPts += 1000000 / 25;

        Sleep(1000 / 25);

        rtspSession->sendG711A((uint8_t*)buf, AUDIO_FRAME_SIZE, audioPts, AUDIO_SAMPLE_RATE);

    }
}

void readAACFile(RtspSession * rtspSession)
{
    FILE *fp = fopen("../test.aac", "rb");
    if (fp == NULL)
    {
        RTSP_LogPrintf("AAC file not exist!");
        exit(-1);
    }

    AACReader *mAACReader = new AACReader();

    while(1)
    {
        char buf[10240] = {0};
        int size = fread(buf, 1, 1024, fp);//从h264文件读1024个字节 (模拟从网络收到h264流)

        if (size <= 0)
        {
            RTSP_LogPrintf("readAACFile finished! \n");

            fseek(fp, 0, SEEK_SET);
            Sleep(1000);
            continue;
        }

        int nCount = mAACReader->inputAACData((uchar*)buf, size);

        while(1)
        {
            //从前面读到的数据中获取一帧音频数据
            AACFrame* aacFrame = mAACReader->getNextFrame();
            if (aacFrame == NULL) break;

//            audioPts += 1000000 / 43;
            audioPts += 6000;

            Sleep(6);

//            rtspSession->sendAACBuffer((uint8_t*)buf, AUDIO_FRAME_SIZE, audioPts, AUDIO_SAMPLE_RATE);
            rtspSession->sendAACBuffer((uint8_t*)aacFrame->buffer, aacFrame->bufferSize, audioPts, 44100);

            FreeAACFrame(aacFrame);
        }
    }
}


int main()
{
    printf("Hello World!\n");

    RtspServer *mRtspServer = new RtspServer();
    mRtspServer->startServer();

    AppConfig::gRtspServer = mRtspServer;

    RtspSession * rtspSession = mRtspServer->newSession("/live/chn1", true, true);

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

//    //启动新的线程读取音频文件（发送aac的暂时还有问题，以后有时间再处理）
//    std::thread([&](RtspSession * rtspSession)
//    {
//        readAACFile(rtspSession);

//    }, rtspSession).detach();

    while(1) Sleep(1000);

    return 0;
}
