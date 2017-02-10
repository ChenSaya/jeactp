#ifndef _THREADLOCK_H_
#define _THREADLOCK_H_

#include <pthread.h>

class CMutexLock
{
public:
	CMutexLock();
	void Lock();
	void Unlock();
	
protected:
	pthread_mutex_t m_tMutex;
};


class CCondLock : public CMutexLock
{
public:
	CCondLock();
	void CondWait();
	void CondSignal();
	
private:
	pthread_cond_t m_tCond;
};


class CAutoMutexLock
{
public:
	CAutoMutexLock(CMutexLock &lock);
	~CAutoMutexLock();
	
private:
	CMutexLock &m_lock;
};


class CAutoCondLock
{
public:
	CAutoCondLock(CCondLock &lock);
	~CAutoCondLock();
	
private:
	CCondLock &m_lock;
};




#endif

