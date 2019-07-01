/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef H264_H
#define H264_H

#include <stdint.h>
#include <stdlib.h>
#include <string>

typedef struct
{
  int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  unsigned max_size;            //! Nal Unit Buffer size
  int forbidden_bit;            //! should be always FALSE
  int nal_reference_idc;        //! NALU_PRIORITY_xxxx
  int nal_unit_type;            //! NALU_TYPE_xxxx
  unsigned char *buf;                    //! contains the first byte followed by the EBSP
  unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;


typedef struct {
    //byte 0
    unsigned char TYPE:5;
    unsigned char NRI:2;
    unsigned char F:1;

} NALU_HEADER; /**//* 1 BYTES */

//为NALU_t结构体分配内存空间
NALU_t *AllocNALU(int buffersize);

//释放
void FreeNALU(NALU_t *n);

/**
 * @brief getNALU 从H264数据中获取一个NALU_t结构体数据
 * @param h264Buf 传入的数据必须是包含起始码的 且只能是完整的一帧数据
 * @return
 */
NALU_t *getNALU(const uint8_t *h264Buf, int len);


#endif // H264_H
