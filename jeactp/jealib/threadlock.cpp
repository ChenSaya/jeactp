#include "threadlock.h"

CMutexLock::CMutexLock()
{
	pthread_mutex_init(&m_tMutex, NULL);
}

void CMutexLock::Lock()
{
	pthread_mutex_lock(&m_tMutex);
}
void CMutexLock::Unlock()
{
	pthread_mutex_unlock(&m_tMutex);
}

////////////////////////////////////////////////

CCondLock::CCondLock()
{
	int ret = pthread_cond_init(&m_tCond, NULL);
	if(0 != ret)
	{
//		LogErr("(%s)pthread_cond_init fail, ret=%d" , __func__, ret);
	}
}

void CCondLock::CondWait()
{
	if(0 != pthread_cond_wait(&m_tCond, &m_tMutex))
	{
//		LogErr("(%s) pthread_cond_wait fail", __func__);
		return;
	}
}

void CCondLock::CondSignal()
{
	pthread_cond_signal(&m_tCond);
}

////////////////////////////////////////////////

CAutoMutexLock::CAutoMutexLock(CMutexLock &lock) : m_lock(lock)
{
	m_lock.Lock();
}
CAutoMutexLock::~CAutoMutexLock()
{
	m_lock.Unlock();
}

////////////////////////////////////////////////

CAutoCondLock::CAutoCondLock(CCondLock &lock) : m_lock(lock)
{
	m_lock.Lock();
}
CAutoCondLock::~CAutoCondLock()
{
	m_lock.Unlock();
}


