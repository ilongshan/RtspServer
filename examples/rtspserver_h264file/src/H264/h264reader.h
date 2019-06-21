/**
 * Ҷ����
 * QQȺ121376426
 * http://blog.yundiantech.com/
 */

#ifndef H264READER_H
#define H264READER_H

#include <stdint.h>
#include <stdlib.h>

#include "H264/h264.h"

//ΪNALU_t�ṹ������ڴ�ռ�
NALU_t *AllocNALU(int buffersize);
//�ͷ�
void FreeNALU(NALU_t *n);

class h264Reader
{
public:
    h264Reader();

    int inputH264Data(uint8_t *buf,int len); //����h264����

    ///��H264�����в��ҳ�һ֡����
    NALU_t* getNextFrame();

private:
    uint8_t *mH264Buffer;
    int mBufferSize;

};

#endif // H264READER_H
