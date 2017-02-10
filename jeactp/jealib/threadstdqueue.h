#ifndef _THREADSTDQUEUE_H_
#define _THREADSTDQUEUE_H_

#include <queue>
#include "threadlock.h"

namespace jealib
{

typedef std::vector<char> QUEUE_ITEM;

class CThreadStdQueue
{
public: 
    CThreadStdQueue();
    ~CThreadStdQueue();

    int Enqueue(const QUEUE_ITEM item);  //push item to queue
    int Dequeue(QUEUE_ITEM &item); //pop queue, and safe the element to item

private:
    std::queue<QUEUE_ITEM> m_queue;
    CCondLock m_oCondLock;
};

}
#endif

