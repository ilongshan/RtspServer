/*
 *  从librtmp偷过来的
 */

#ifndef __RTSP_LOG_H__
#define __RTSP_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* Enable this to get full debugging output */
/* #define _DEBUG */

#ifdef _DEBUG
#undef NODEBUG
#endif

typedef enum
{ RTSP_LOGCRIT=0, RTSP_LOGERROR, RTSP_LOGWARNING, RTSP_LOGINFO,
  RTSP_LOGDEBUG, RTSP_LOGDEBUG2, RTSP_LOGALL
} RTSP_LogLevel;

extern RTSP_LogLevel RTSP_debuglevel;

typedef void (RTSP_LogCallback)(int level, const char *fmt, va_list);
void RTSP_LogSetCallback(RTSP_LogCallback *cb);
void RTSP_LogSetOutput(FILE *file);
void RTSP_LogPrintf(const char *format, ...);
void RTSP_LogStatus(const char *format, ...);
void RTSP_Log(int level, const char *format, ...);
void RTSP_LogHex(int level, const uint8_t *data, unsigned long len);
void RTSP_LogHexString(int level, const uint8_t *data, unsigned long len);
void RTSP_LogSetLevel(RTSP_LogLevel lvl);
RTSP_LogLevel RTSP_LogGetLevel(void);

#ifdef __cplusplus
}
#endif

#endif
