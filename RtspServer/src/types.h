#ifndef TYPE_H
#define TYPE_H

#if defined(WIN32)
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <pthread.h>
#endif

#include <stdio.h>

#define dbg(fmt, ...) do {printf("[DEBUG %s:%d:%s] " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);} while(0)
#define info(fmt, ...) do {printf("[INFO  %s:%d:%s] " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);} while(0)
#define warn(fmt, ...) do {printf("[WARN  %s:%d:%s] " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);} while(0)
#define err(fmt, ...) do {printf("[ERROR %s:%d:%s] " fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);} while(0)

#ifdef WIN32
//#include <windows.h>
#include <winsock2.h>
#define MSG_DONTWAIT 0
#define usleep(x) Sleep((x)/1000)
#define snprintf _snprintf
typedef int socklen_t;
#endif

#ifdef __LINUX__
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define closesocket(x) close(x)
#endif

#endif // TYPE_H
