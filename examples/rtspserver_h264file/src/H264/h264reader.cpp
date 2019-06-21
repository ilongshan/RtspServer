/**
 * Ҷ����
 * QQȺ121376426
 * http://blog.yundiantech.com/
 */

#include "h264reader.h"

#include <stdlib.h>
#include <string.h>

//ΪNALU_t�ṹ������ڴ�ռ�
NALU_t *AllocNALU(int buffersize)
{
    NALU_t *n = NULL;

    n = (NALU_t*)malloc (sizeof(NALU_t));
    n->max_size = buffersize;	//Assign buffer size
    n->buf = (unsigned char*)malloc (buffersize);
    n->len = buffersize;

    return n;
}

//�ͷ�
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
    ///��ʼ��һ���ڴ� ������ʱ���h264����
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

    /*����h264�ļ�������  ����ֽ����� ֱ������h264��֡ͷ ��Ϊ��ȡ����������һ֡h264��Ƶ����*/

///    ������ʼ��startcode��������ʽ��3�ֽڵ�0x000001��4�ֽڵ�0x00000001
///    3�ֽڵ�0x000001ֻ��һ�ֳ�����ʹ�ã�����һ��������֡����Ϊ���slice��ʱ��
///    ������Щslice��naluʹ��3�ֽ���ʼ�롣���ೡ�϶���4�ֽڵġ�

    ///���Ȳ��ҵ�һ����ʼ��

    int pos = 0; //��¼��ǰ���������ƫ����
    int StartCode = 0;

    while(1)
    {
        unsigned char* Buf = mH264Buffer + pos;
        int lenth = mBufferSize - pos; //ʣ��û�д�������ݳ���
        if (lenth <= 4)
        {
            return NULL;
        }

        ///������ʼ��(0x00000001)

        if(Buf[0]==0 && Buf[1]==0 && Buf[2] ==0 && Buf[3] ==1)
         //Check whether buf is 0x00000001
        {
            StartCode = 4;
            break;
        }
        else
        {
            //���� �������һ���ֽ�
            pos++;
        }
    }


    ///Ȼ�������һ����ʼ����ҵ�һ����ʼ��

    int pos_2 = pos + StartCode; //��¼��ǰ���������ƫ����
    int StartCode_2 = 0;

    while(1)
    {
        unsigned char* Buf = mH264Buffer + pos_2;
        int lenth = mBufferSize - pos_2; //ʣ��û�д�������ݳ���
        if (lenth <= 4)
        {
            return NULL;
        }

        ///������ʼ��(0x00000001)

        if(Buf[0]==0 && Buf[1]==0 && Buf[2] ==0 && Buf[3] ==1)
         //Check whether buf is 0x00000001
        {
            StartCode_2 = 4;
            break;
        }
        else
        {
            //���� �������һ���ֽ�
            pos_2++;
        }
    }

    /// ���� pos��pos_2֮������ݾ���һ֡������
    /// ����ȡ����

    ///ע�⣺���������ȥ������ʼ��
    unsigned char* Buf = mH264Buffer + pos + StartCode; //��֡���ݵ���ʼ����(��������ʼ��)
    int naluSize = pos_2 - pos - StartCode; //nalu���ݴ�С ��������ʼ��

//    NALU_HEADER *nalu_header = (NALU_HEADER *)Buf;

    NALU_t * nalu = AllocNALU(naluSize);//����nal ��Դ

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


    /// ����һ֡����ȥ��
    /// �Ѻ�һ֡���ݸ�������
    int leftSize = mBufferSize - pos_2;
    memcpy(mH264Buffer, mH264Buffer + pos_2, leftSize);
    mBufferSize = leftSize;

    return nalu;
}
