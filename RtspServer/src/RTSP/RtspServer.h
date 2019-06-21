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
     * @brief startServer ��������rtsp������
     * @param port  [in] rtsp�����˿�
     * @return �Ƿ������ɹ�
     */
    bool startServer(int port = 554);

    /**
     * @brief newSession �½�һ��RTSP�Ự
     * @param name [in] �Ự���� �磺"/live/chn1"  ��ô���ʵĵ�ַ����:rtsp://xxxx:554/live/chn1
     * @return
     */
    RtspSession *newSession(std::string name);

    ///��ȡRtsp�Ự�б�
    std::list<RtspSession*> getRtspSessonList(){return mRtspSessionList;}

protected:
    void threadStart();
    void threadStop();
    void threadFunc();

private:
    int mSockFd; //tcp������fd

    struct sockaddr_in localaddr;
    struct sockaddr_in remoteaddr;

    std::list<RtspSession*> mRtspSessionList; //Rtsp�Ự�б�
    std::list<RtspClient*> mRtspClientList; //���ӽ����Ŀͻ����б�

};

#endif // RTSPSERVER_H
