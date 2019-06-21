/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#include "h264reader.h"

#include <stdlib.h>
#include <string.h>

//为NALU_t结构体分配内存空间
NALU_t *AllocNALU(int buffersize)
{
    NALU_t *n = NULL;

    n = (NALU_t*)malloc (sizeof(NALU_t));
    n->max_size = buffersize;	//Assign buffer size
    n->buf = (unsigned char*)malloc (buffersize);
    n->len = buffersize;

    return n;
}

//释放
void FreeNALU(NALU_t *n)
{
  if (n)
  {
    if (n->buf)
    {
      free(n->buf);
      n->buf=NULL;
    }
    free (n);
  }
}


h264Reader::h264Reader()
{
    ///初始化一段内存 用于临时存放h264数据
    mH264Buffer = (uint8_t*)malloc(1024*1024*10);
    mBufferSize = 0;
}

int h264Reader::inputH264Data(uint8_t *buf, int len)
{
    memcpy(mH264Buffer + mBufferSize, buf, len);
    mBufferSize += len;

    return mBufferSize;
}

NALU_t* h264Reader::getNextFrame()
{

    /*根据h264文件的特性  逐个字节搜索 直到遇到h264的帧头 视为获取到了完整的一帧h264视频数据*/

///    关于起始码startcode的两种形式：3字节的0x000001和4字节的0x00000001
///    3字节的0x000001只有一种场合下使用，就是一个完整的帧被编为多个slice的时候，
///    包含这些slice的nalu使用3字节起始码。其余场合都是4字节的。

    ///首先查找第一个起始码

    int pos = 0; //记录当前处理的数据偏移量
    int StartCode = 0;

    while(1)
    {
        unsigned char* Buf = mH264Buffer + pos;
        int lenth = mBufferSize - pos; //剩余没有处理的数据长度
        if (lenth <= 4)
        {
            return NULL;
        }

        ///查找起始码(0x00000001)

        if(Buf[0]==0 && Buf[1]==0 && Buf[2] ==0 && Buf[3] ==1)
         //Check whether buf is 0x00000001
        {
            StartCode = 4;
            break;
        }
        else
        {
            //否则 往后查找一个字节
            pos++;
        }
    }


    ///然后查找下一个起始码查找第一个起始码

    int pos_2 = pos + StartCode; //记录当前处理的数据偏移量
    int StartCode_2 = 0;

    while(1)
    {
        unsigned char* Buf = mH264Buffer + pos_2;
        int lenth = mBufferSize - pos_2; //剩余没有处理的数据长度
        if (lenth <= 4)
        {
            return NULL;
        }

        ///查找起始码(0x00000001)

        if(Buf[0]==0 && Buf[1]==0 && Buf[2] ==0 && Buf[3] ==1)
         //Check whether buf is 0x00000001
        {
            StartCode_2 = 4;
            break;
        }
        else
        {
            //否则 往后查找一个字节
            pos_2++;
        }
    }

    /// 现在 pos和pos_2之间的数据就是一帧数据了
    /// 把他取出来

    ///注意：这里的数据去掉了起始码
    unsigned char* Buf = mH264Buffer + pos + StartCode; //这帧数据的起始数据(不包含起始码)
    int naluSize = pos_2 - pos - StartCode; //nalu数据大小 不包含起始码

//    NALU_HEADER *nalu_header = (NALU_HEADER *)Buf;

    NALU_t * nalu = AllocNALU(naluSize);//分配nal 资源

    nalu->startcodeprefix_len = StartCode;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    nalu->len = naluSize;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
//    nalu->forbidden_bit = 0;            //! should be always FALSE
//    nalu->nal_reference_idc = nalu_header->NRI;        //! NALU_PRIORITY_xxxx
//    nalu->nal_unit_type = nalu_header->TYPE;            //! NALU_TYPE_xxxx
    nalu->lost_packets = false;  //! true, if packet loss is detected


    memcpy(nalu->buf, Buf, naluSize);  //! contains the first byte followed by the EBSP

    nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit


    /// 将这一帧数据去掉
    /// 把后一帧数据覆盖上来
    int leftSize = mBufferSize - pos_2;
    memcpy(mH264Buffer, mH264Buffer + pos_2, leftSize);
    mBufferSize = leftSize;

    return nalu;
}
