#include "RtspServer.h"

#include <list>

#include "Log/log.h"
#include "AppConfig.h"
#include "SDP/RtspSdp.h"

RtspServer::RtspServer()
{

}

RtspSession * RtspServer::newSession(std::string name)
{
    RtspSession *mRtspSession = new RtspSession;
    mRtspSession->setSessionPath(name);
    mRtspSessionList.push_back(mRtspSession);
    return mRtspSession;
}

bool RtspServer::startServer(int port)
{
#ifdef WIN32
    WSADATA ws;
    WSAStartup(MAKEWORD(2,2), &ws);
#endif

    int ret = 0;

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0)
    {
        RTSP_LogPrintf("create socket failed : %s\n", strerror(errno));
        return false;
    }

    // �󶨵�ַ�Ͷ˿�
    memset(&localaddr, 0, sizeof(localaddr));
    localaddr.sin_family = AF_INET;
    localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localaddr.sin_port = htons(port);

    ret = bind(socket_fd, (struct sockaddr*)&localaddr, sizeof(localaddr));
    if (ret < 0)
    {
        RTSP_LogPrintf("bind socket to address failed : %s\n", strerror(errno));
        closesocket(socket_fd);
        return false;
    }

    // �����׽ӿڶ���
    ret = listen(socket_fd, 100); //���������Ŀ�������(100)��client�˽��յ�ECONNREFUSED�Ĵ���
    if (ret < 0)
    {
        RTSP_LogPrintf("listen socket failed : %s\n", strerror(errno));
        closesocket(socket_fd);
        return false;
    }

    ////KeepAliveʵ�֣���λ��
    //�������Ҫ����ACE,���û�а���ACE,������õ���ACE�����ĳ�linux��Ӧ�Ľӿ�
    int keepAlive = 1;//�趨KeepAlive
    int keepIdle = 5;//��ʼ�״�KeepAlive̽��ǰ��TCP�ձ�ʱ��
    int keepInterval = 5;//����KeepAlive̽����ʱ����
    int keepCount = 3;//�ж��Ͽ�ǰ��KeepAlive̽�����
//    if(setsockopt(socket_fd,SOL_SOCKET,SO_KEEPALIVE,(void*)&keepAlive,sizeof(keepAlive)) == -1)
    if(setsockopt(socket_fd,SOL_SOCKET,SO_KEEPALIVE,(char*)&keepAlive,sizeof(keepAlive)) == -1)
    {
        fprintf(stderr," setsockopt SO_KEEPALIVE error!\n");
    }

//    if(setsockopt(socket_fd,SOL_TCP,TCP_KEEPIDLE,(void *)&keepIdle,sizeof(keepIdle)) == -1)
//    if(setsockopt(socket_fd,SOL_TCP,TCP_KEEPIDLE,(char *)&keepIdle,sizeof(keepIdle)) == -1)
//    {
//        fprintf(stderr," setsockopt TCP_KEEPIDLE error!\n");
//    }

////    if(setsockopt(socket_fd,SOL_TCP,TCP_KEEPINTVL,(void *)&keepInterval,sizeof(keepInterval)) == -1)
//    if(setsockopt(socket_fd,SOL_TCP,TCP_KEEPINTVL,(char *)&keepInterval,sizeof(keepInterval)) == -1)
//    {
//        fprintf(stderr," setsockopt TCP_KEEPINTVL error!\n");
//    }

//    if(setsockopt(socket_fd,SOL_TCP,TCP_KEEPCNT,(void *)&keepCount,sizeof(keepCount)) == -1)
//    {
//        fprintf(stderr," setsockopt TCP_KEEPCNT error!\n");
//    }

    RTSP_LogPrintf("rtsp server demo starting on port %d\n", port);

    mSockFd = socket_fd;

    ///���������߳�
    this->start();

    return true;
}

void RtspServer::threadStart()
{

}

void RtspServer::threadStop()
{

}

void RtspServer::threadFunc()
{
    do
    {
        const int &socket_fd = mSockFd;

        fd_set fdsr;  //����ļ�����������
        FD_ZERO(&fdsr);
        FD_SET(socket_fd, &fdsr);

        int maxsock = socket_fd;//����ļ������������������ļ���

        for (const RtspClient* rtspClient : mRtspClientList)
        {
            const int &fd = ((RtspClient*)rtspClient)->getSocketFd();
            FD_SET(fd, &fdsr);
            if (fd>maxsock)
            {
                maxsock = fd;
            }
        }

//        struct timeval tv; //Select��ʱ���ص�ʱ�䡣
//        tv.tv_sec = 30; // ��ʱ����30��
//        tv.tv_usec = 0;
//        int ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);
        int ret = select(maxsock + 1, &fdsr, NULL, NULL, NULL);

        fprintf(stderr,"select %d\n",ret);

        if (ret < 0)
        {
            RTSP_LogPrintf("select error! ret=%d\n", ret);
            AppConfig::mSleep(100);
            break;
        }
        else if (ret == 0)
        {
            RTSP_LogPrintf("selet timeout\n");
            break;
        }

        // ����Ƿ��������ӽ���������������ӽ������������ӣ�������socket��
        //�����뵽����ļ������������С�
        if (FD_ISSET(socket_fd, &fdsr))
        {
            int len = sizeof(remoteaddr);

//            int client_sockfd = accept(socket_fd,(struct sockaddr *)&remoteaddr, (socklen_t*)&len);
            int client_sockfd = accept(socket_fd,(struct sockaddr *)&remoteaddr, &len);

            if (client_sockfd > 0)
            {
                std::cout << "one client in fd is:"<<client_sockfd << std::endl;
                fprintf(stderr,"one client in fd is:%d\n",client_sockfd);

                RtspClient *rtspClient = new RtspClient();

                rtspClient->setSocketFd(client_sockfd);

                mRtspClientList.push_back(rtspClient);

                FD_SET(client_sockfd, &fdsr);

                if (client_sockfd > maxsock)
                {
                    maxsock = client_sockfd;
                }
            }
            else
            {
                RTSP_LogPrintf("accept socket error! fd = %d\n",client_sockfd);
            }

            continue;
        }

        ///�ж��Ƿ�������Ϣ
        std::list<RtspClient*>::iterator iter;
        for(iter=mRtspClientList.begin(); iter!=mRtspClientList.end();)
        {
            std::list<RtspClient*>::iterator iter_e=iter++;

            RtspClient *rtspClient = *iter_e;

            const int client_sockfd = rtspClient->getSocketFd();

            ///���fdset��ϵ���ļ����fd�Ƿ�ɶ�д�� >0��ʾ�ɶ�д��
            if (!FD_ISSET(client_sockfd, &fdsr))
            {
                continue;
            }

            char data[2048] = {0};
            int size = recv(client_sockfd, (char *)data, 2048, 0);
            if(size < 0)
            {
                mRtspClientList.erase(iter_e);

                closesocket(client_sockfd);

                for (RtspSession* session : mRtspSessionList)
                {
                    session->removeRtspClient(rtspClient);
                }

                delete rtspClient;

                RTSP_LogPrintf("server recieve data failed! close it.--\n");
            }
            else if(size ==0)
            {
                mRtspClientList.erase(iter_e);

                closesocket(client_sockfd);

                for (RtspSession* session : mRtspSessionList)
                {
                    session->removeRtspClient(rtspClient);
                }

                delete rtspClient;

                RTSP_LogPrintf("one client is closed! fd is: %d--\n",client_sockfd);
            }
            else
            {
                RTSP_LogPrintf("recv data: %s \n", data);

                rtspClient->dealwithReceiveBuffer(data, size);
            }
        }

    }while(0);

}
