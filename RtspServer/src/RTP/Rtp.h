/**
 * 叶海辉
 * QQ群121376426
 * http://blog.yundiantech.com/
 */

#ifndef RTP_H
#define RTP_H

#pragma pack(1)

typedef struct
{
    /**//* byte 0 */
    unsigned char csrc_len:4;        /**//* expect 0 */
    unsigned char extension:1;        /**//* expect 1, see RTP_OP below */
    unsigned char padding:1;        /**//* expect 0 */
    unsigned char version:2;        /**//* expect 2 */
    /**//* byte 1 */
    unsigned char payload:7;        /**//* RTP_PAYLOAD_RTSP */
    unsigned char marker:1;        /**//* expect 1 */
    /**//* bytes 2, 3 */
    unsigned short seq_no;
    /**//* bytes 4-7 */
    unsigned  long timestamp;
    /**//* bytes 8-11 */
    unsigned long ssrc;            /**//* stream number is used here. */
} RTP_FIXED_HEADER;

//struct rtphdr
//{
//#ifdef __BIG_ENDIAN__
//	uint16_t v:2;
//	uint16_t p:1;
//	uint16_t x:1;
//	uint16_t cc:4;
//	uint16_t m:1;
//	uint16_t pt:7;
//#else
//	uint16_t cc:4;
//	uint16_t x:1;
//	uint16_t p:1;
//	uint16_t v:2;
//	uint16_t pt:7;
//	uint16_t m:1;
//#endif
//	uint16_t seq;
//	uint32_t ts;
//	uint32_t ssrc;
//};

//#define RTPHDR_SIZE (12)

typedef struct {
    //byte 0
    unsigned char TYPE:5;
    unsigned char NRI:2;
    unsigned char F:1;


} FU_INDICATOR; /**//* 1 BYTES */

typedef struct {
    //byte 0
    unsigned char TYPE:5;
        unsigned char R:1;
        unsigned char E:1;
        unsigned char S:1;
} FU_HEADER; /**//* 1 BYTES */

#pragma pack ()

#endif // RTP_H
