/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef H264READER_H
#define H264READER_H

#include <stdint.h>
#include <stdlib.h>

#include "H264/h264.h"

//为NALU_t结构体分配内存空间
NALU_t *AllocNALU(int buffersize);
//释放
void FreeNALU(NALU_t *n);

class h264Reader
{
public:
    h264Reader();

    int inputH264Data(uint8_t *buf,int len); //输入h264数据

    ///从H264数据中查找出一帧数据
    NALU_t* getNextFrame();

private:
    uint8_t *mH264Buffer;
    int mBufferSize;

};

#endif // H264READER_H
