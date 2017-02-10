#include "threadstdqueue.h"
#include "simplelog.h"

namespace jealib
{

CThreadStdQueue::CThreadStdQueue(){}

CThreadStdQueue::~CThreadStdQueue(){}

int CThreadStdQueue::Enqueue(const QUEUE_ITEM item)
{
    m_oCondLock.Lock();

    m_queue.push(item);

    int iSize = m_queue.size();
    LOG << LOGBEG << "m_queue.size: " << iSize << LOGEND;    
 
    m_oCondLock.Unlock();
    m_oCondLock.CondSignal();
    return 0;
}

int CThreadStdQueue::Dequeue(QUEUE_ITEM &item)
{
    CAutoCondLock autoCondLock(m_oCondLock);
    while (0 == m_queue.size())
    {
        m_oCondLock.CondWait();
    }

    item = m_queue.front();
    m_queue.pop();

    return 0;
}

}

