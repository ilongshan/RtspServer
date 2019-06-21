#ifndef COMMONTHREAD_H
#define COMMONTHREAD_H

#include <thread>

/**
 * @brief The CommonThread class
 * һֱѭ��ִ��һ���������߳�
 */

class CommonThread
{
public:
    CommonThread();
    ~CommonThread();

public:
    void start();

    void stop(bool isWait = false);

protected:
    bool mIsStop;
    bool mIsThreadRunning;

    void run();

protected:
    virtual void threadStart();
    virtual void threadStop();
    virtual void threadFunc() = 0;

};

#endif // COMMONTHREAD_H
