#ifndef RTSPSDP_H
#define RTSPSDP_H

#include <stdint.h>

class RtspSdp
{
public:
    RtspSdp();

    static int buildSimpleSdp (char *sdpbuf, int maxlen, const char *uri, int has_video, int has_audio,
        const uint8_t *sps, int sps_len, const uint8_t *pps, int pps_len);

};

#endif // RTSPSDP_H
