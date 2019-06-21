#ifndef H264DECODER_H
#define H264DECODER_H

typedef  unsigned int UINT;
typedef  unsigned char BYTE;
typedef  unsigned long DWORD;

class h264Decoder
{
public:
    h264Decoder();

    static bool decodeSps(BYTE * buf,unsigned int nLen,int &width,int &height,int &fps);

};

#endif // H264DECODER_H
