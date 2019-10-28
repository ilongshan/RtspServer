#include "RtspSdp.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

RtspSdp::RtspSdp()
{

}

/*****************************************************************************
* b64_encode: Stolen from VLC's http.c.
* Simplified by Michael.
* Fixed edge cases and made it work from data (vs. strings) by Ryan.
*****************************************************************************/
static char *base64_encode (char *out, int out_size, const uint8_t *in, int in_size)
{
    static const char b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *ret, *dst;
    unsigned i_bits = 0;
    int i_shift = 0;
    int bytes_remaining = in_size;

#define __UINT_MAX (~0lu)
#define __BASE64_SIZE(x)  (((x)+2) / 3 * 4 + 1)
#define __RB32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])
    if (in_size >= __UINT_MAX / 4 ||
        out_size < __BASE64_SIZE(in_size))
        return NULL;
    ret = dst = out;
    while (bytes_remaining > 3) {
        i_bits = __RB32(in);
        in += 3; bytes_remaining -= 3;
        *dst++ = b64[ i_bits>>26        ];
        *dst++ = b64[(i_bits>>20) & 0x3F];
        *dst++ = b64[(i_bits>>14) & 0x3F];
        *dst++ = b64[(i_bits>>8 ) & 0x3F];
    }
    i_bits = 0;
    while (bytes_remaining) {
        i_bits = (i_bits << 8) + *in++;
        bytes_remaining--;
        i_shift += 8;
    }
    while (i_shift > 0) {
        *dst++ = b64[(i_bits << 6 >> i_shift) & 0x3f];
        i_shift -= 6;
    }
    while ((dst - ret) & 3)
        *dst++ = '=';
    *dst = '\0';

    return ret;
}

int RtspSdp::buildSimpleSdp (char *sdpbuf, int maxlen, const char *uri, int has_video, int has_audio,
    const uint8_t *sps, int sps_len, const uint8_t *pps, int pps_len)
{
    char *p = sdpbuf;

    p += sprintf(p, "v=0\r\n");
    p += sprintf(p, "o=- 0 0 IN IP4 0.0.0.0\r\n");
//    p += sprintf(p, "s=h264+pcm_alaw\r\n");
    p += sprintf(p, "t=0 0\r\n");
    p += sprintf(p, "a=control:%s\r\n", uri ? uri : "*");
    p += sprintf(p, "a=range:npt=0-\r\n");

    if (has_video) {
        p += sprintf(p, "m=video 0 RTP/AVP 96\r\n");
        p += sprintf(p, "c=IN IP4 0.0.0.0\r\n");
        p += sprintf(p, "a=rtpmap:96 H264/90000\r\n");
        if (sps && pps && sps_len > 0 && pps_len > 0) {
            if (sps[0] == 0 && sps[1] == 0 && sps[2] == 1) {
                sps += 3;
                sps_len -= 3;
            }
            if (sps[0] == 0 && sps[1] == 0 && sps[2] == 0 && sps[3] == 1) {
                sps += 4;
                sps_len -= 4;
            }
            if (pps[0] == 0 && pps[1] == 0 && pps[2] == 1) {
                pps += 3;
                pps_len -= 3;
            }
            if (pps[0] == 0 && pps[1] == 0 && pps[2] == 0 && pps[3] == 1) {
                pps += 4;
                pps_len -= 4;
            }
            p += sprintf(p, "a=fmtp:96 packetization-mode=1;sprop-parameter-sets=");
            base64_encode(p, (maxlen - (p - sdpbuf)), sps, sps_len);
            p += strlen(p);
            p += sprintf(p, ",");
            base64_encode(p, (maxlen - (p - sdpbuf)), pps, pps_len);
            p += strlen(p);
            p += sprintf(p, "\r\n");
        } else {
            p += sprintf(p, "a=fmtp:96 packetization-mode=1\r\n");
        }
        if (uri)
            p += sprintf(p, "a=control:%s/track1\r\n", uri);
        else
            p += sprintf(p, "a=control:track1\r\n");
    }

//    if (has_audio)
//    {
////		p += sprintf(p, "m=audio 0 RTP/AVP 8\r\n"); //PCMA/8000/1 (G711A)
//        p += sprintf(p, "m=audio 0 RTP/AVP 97\r\n");
//        p += sprintf(p, "c=IN IP4 0.0.0.0\r\n");
//        p += sprintf(p, "a=rtpmap:97 PCMA/8000/1\r\n");
//        if (uri)
//            p += sprintf(p, "a=control:%s/track2\r\n", uri);
//        else
//            p += sprintf(p, "a=control:track2\r\n");
//    }

    if (has_audio)
    {
//        config=1188
        int profile = 1;
        int samplingFrequencyIndex = 3; //48000
        int channelConfiguration = 1; //当通道

        unsigned char audioSpecificConfig[2];
        uint8_t const audioObjectType = profile + 1;
        audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
        audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channelConfiguration<<3);

        char fConfigStr[6] = {0};
        sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);
        fprintf(stderr, "\n\n\n############ \n %s \n ############### \n\n\n", fConfigStr);

//        p += sprintf(p, "m=audio 0 RTP/AVP 97\r\n");
//        p += sprintf(p, "c=IN IP4 0.0.0.0\r\n");
//        p += sprintf(p, "a=rtpmap:97 mpeg4-generic/48000/1\r\n");
//        p += sprintf(p, "a=fmtp:97 streamtype=5; profile-level-id=15; mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=118856e500; Profile=1;\r\n");
//        p += sprintf(p, "a=Media_header:MEDIAINFO=494D4B4801010000040000010120011080BB000000FA000000000000000000000000000000000000;\r\n");

        p += sprintf(p, "m=audio 0 RTP/AVP 97\r\n");
        p += sprintf(p, "c=IN IP4 0.0.0.0\r\n");
        p += sprintf(p, "a=rtpmap:97 mpeg4-generic/44100/2\r\n");
        p += sprintf(p, "a=fmtp:97 SizeLength=13;\r\n");

        if (uri)
            p += sprintf(p, "a=control:%s/track2\r\n", uri);
        else
            p += sprintf(p, "a=control:track2\r\n");
    }

    return (p - sdpbuf);
}
