#ifndef RTSPSERVER_H
#define RTSPSERVER_H

#include <list>

#include "Thread/CommonThread.h"
#include "types.h"

#include "RtspSession.h"
#include "RtspClient.h"

class RtspServer : public CommonThread
{
public:
    RtspServer();

    /**
     * @brief startServer 启动监听rtsp服务器
     * @param port  [in] rtsp监听端口
     * @return 是否启动成功
     */
    bool startServer(int port = 554);

    /**
     * @brief newSession 新建一个RTSP会话
     * @param name [in] 会话名称 如："/live/chn1"  那么访问的地址就是:rtsp://xxxx:554/live/chn1
     * @return
     */
    RtspSession *newSession(std::string name, bool isHasVideo, bool isHasAudio);

    ///获取Rtsp会话列表
    std::list<RtspSession*> getRtspSessonList(){return mRtspSessionList;}

protected:
    void threadStart();
    void threadStop();
    void threadFunc();

private:
    int mSockFd; //tcp服务器fd

    struct sockaddr_in localaddr;
    struct sockaddr_in remoteaddr;

    std::list<RtspSession*> mRtspSessionList; //Rtsp会话列表
    std::list<RtspClient*> mRtspClientList; //连接进来的客户端列表

};

#endif // RTSPSERVER_H
