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

    // 绑定地址和端口
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

    // 建立套接口队列
    ret = listen(socket_fd, 100); //如果连接数目达此上限(100)则client端将收到ECONNREFUSED的错误。
    if (ret < 0)
    {
        RTSP_LogPrintf("listen socket failed : %s\n", strerror(errno));
        closesocket(socket_fd);
        return false;
    }

    ////KeepAlive实现，单位秒
    //下面代码要求有ACE,如果没有包含ACE,则请把用到的ACE函数改成linux相应的接口
    int keepAlive = 1;//设定KeepAlive
    int keepIdle = 5;//开始首次KeepAlive探测前的TCP空闭时间
    int keepInterval = 5;//两次KeepAlive探测间的时间间隔
    int keepCount = 3;//判定断开前的KeepAlive探测次数
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

    ///启动监听线程
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

        fd_set fdsr;  //监控文件描述符集合
        FD_ZERO(&fdsr);
        FD_SET(socket_fd, &fdsr);

        int maxsock = socket_fd;//监控文件描述符集合中最大的文件号

        for (const RtspClient* rtspClient : mRtspClientList)
        {
            const int &fd = ((RtspClient*)rtspClient)->getSocketFd();
            FD_SET(fd, &fdsr);
            if (fd>maxsock)
            {
                maxsock = fd;
            }
        }

//        struct timeval tv; //Select超时返回的时间。
//        tv.tv_sec = 30; // 超时设置30秒
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

        // 检查是否有新连接进来，如果有新连接进来，接收连接，生成新socket，
        //并加入到监控文件描述符集合中。
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

        ///判断是否有新消息
        std::list<RtspClient*>::iterator iter;
        for(iter=mRtspClientList.begin(); iter!=mRtspClientList.end();)
        {
            std::list<RtspClient*>::iterator iter_e=iter++;

            RtspClient *rtspClient = *iter_e;

            const int client_sockfd = rtspClient->getSocketFd();

            ///检查fdset联系的文件句柄fd是否可读写， >0表示可读写。
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
