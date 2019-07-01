#include "h264.h"

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

NALU_t *getNALU(const uint8_t * h264Buf, int len)
{
///    关于起始码startcode的两种形式：3字节的0x000001和4字节的0x00000001
///    3字节的0x000001只有一种场合下使用，就是一个完整的帧被编为多个slice的时候，
///    包含这些slice的nalu使用3字节起始码。其余场合都是4字节的。

    uint8_t *Buf = (uint8_t *)h264Buf;
    int StartCode = 0;

    if(Buf[0]==0 && Buf[1]==0 && Buf[2] ==0 && Buf[3] ==1)
     //Check whether buf is 0x00000001
    {
        StartCode = 4;
    }
    else if(Buf[0]==0 && Buf[1]==0 && Buf[2] ==1)
    { //Check whether buf is 0x000001
        StartCode = 3;
    }
    else
    {
        //否则 往后查找一个字节
        return nullptr;
    }

    int naluSize = len - StartCode; //nalu数据大小 不包含起始码

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

    return nalu;
}
