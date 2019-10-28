#ifndef AACREADER_H
#define AACREADER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
sampling_frequency_index sampling frequeny [Hz]
0x0                           96000
0x1                           88200
0x2                           64000
0x3                           48000
0x4                           44100
0x5                           32000
0x6                           24000
0x7                           22050
0x8                           16000
0x9                           2000
0xa                           11025
0xb                           8000
0xc                           reserved
0xd                           reserved
0xe                           reserved
0xf                           reserved
*/
typedef struct
{
    unsigned int syncword;  //12 bslbf 同步字The bit string ‘1111 1111 1111’，说明一个ADTS帧的开始
    unsigned int id;        //1 bslbf   MPEG 标示符, 设置为1
    unsigned int layer;     //2 uimsbf Indicates which layer is used. Set to ‘00’
    unsigned int protection_absent;  //1 bslbf  表示是否误码校验
    unsigned int profile;            //2 uimsbf  表示使用哪个级别的AAC，如01 Low Complexity(LC)--- AACLC
    unsigned int sf_index;           //4 uimsbf  表示使用的采样率下标
    unsigned int private_bit;        //1 bslbf
    unsigned int channel_configuration;  //3 uimsbf  表示声道数
    unsigned int original;               //1 bslbf
    unsigned int home;                   //1 bslbf
    /*下面的为改变的参数即每一帧都不同*/
    unsigned int copyright_identification_bit;   //1 bslbf
    unsigned int copyright_identification_start; //1 bslbf
    unsigned int aac_frame_length;               // 13 bslbf  一个ADTS帧的长度包括ADTS头和raw data block
    unsigned int adts_buffer_fullness;           //11 bslbf     0x7FF 说明是码率可变的码流

    /*no_raw_data_blocks_in_frame 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧.
    所以说number_of_raw_data_blocks_in_frame == 0
    表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
    */
    unsigned int no_raw_data_blocks_in_frame;    //2 uimsfb
} ADTS_HEADER;


struct AACFrame
{
    ADTS_HEADER adtsHeader;
    uint8_t *buffer;
    int bufferSize; //aac数据长度（不包括adts头的大小）
};

//为AACFrame结构体分配内存空间
AACFrame *AllocAACFrame(int buffersize);

//释放
void FreeAACFrame(AACFrame *frame);

class AACReader
{
public:
    AACReader();

    int inputAACData(uint8_t *buf, int len); //输入aac数据

    ///从AAC数据中查找出一帧数据
    AACFrame* getNextFrame();

private:
    uint8_t *mAACBuffer;
    int mBufferSize;

    ///读取adts头
    bool ReadAdtsHeader(unsigned char * adts_headerbuf, ADTS_HEADER *adts);

};

#endif // AACREADER_H
