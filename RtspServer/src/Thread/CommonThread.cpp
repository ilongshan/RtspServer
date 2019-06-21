#include "CommonThread.h"

CommonThread::CommonThread()
{
	mIsStop = true;
}

CommonThread::~CommonThread()
{

}

void CommonThread::start()
{
    mIsStop = false;

    //启动新的线程
    std::thread([&](CommonThread *pointer)
    {
        pointer->run();

    }, this).detach();
}

void CommonThread::stop(bool isWait)
{
    mIsStop = true;

    if (isWait)
    {
        while(mIsThreadRunning)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void CommonThread::threadStart()
{

}

void CommonThread::threadStop()
{

}


void CommonThread::run()
{
    mIsThreadRunning = true;

    threadStart();

    while(!mIsStop)
    {
        threadFunc();
    }

    threadStop();

    mIsThreadRunning = false;
}
