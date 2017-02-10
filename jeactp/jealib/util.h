#ifndef _UTIL_H_20160612_
#define _UTIL_H_20160612_

#if defined(WIN32)
#include <windows.h>
#elif defined(_POSIX_THREADS)
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#endif


namespace jealib 
{
    inline void Sleep(unsigned int msec) 
    {
    #if defined(WIN32)
        ::Sleep(msec);
    #elif defined(_POSIX_THREADS)
        timespec tm;
        tm.tv_sec = msec / 1000;
        tm.tv_nsec = msec % 1000 * 1000;
        nanosleep(&tm, 0);
    #endif
    }

    inline unsigned int GetTickCount() 
    {
    #if defined(WIN32)
        return ::GetTickCount();
    #elif defined(_POSIX_THREADS)		
        timeval tv;
        gettimeofday(&tv, 0);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000; 
    #endif		
    }

    inline unsigned int GetCurrentThreadId() 
    {
    #if defined(WIN32)
        return ::GetCurrentThreadId();
    #elif defined(_POSIX_THREADS)
        return pthread_self();
    #endif
    }


    inline unsigned int GetCostTime(unsigned int now, unsigned int last) 
    {
        if (now >= last) return now - last;
        else return 0xffffffff - last + now;
    }

    inline std::string GetDate_yyyymmdd()
    {
        time_t now = time(0);
        tm * pTime = localtime(&now);
        std::stringstream oss;
        oss << pTime->tm_year + 1900
            << std::setw(2) << std::setfill('0') << pTime->tm_mon + 1
            << std::setw(2) << std::setfill('0') << pTime->tm_mday
        ;
        return oss.str();
    }

    inline std::string GetCurrentTimeWithMS()
    {
        timeval tv;
        gettimeofday(&tv, 0);
        time_t tt = tv.tv_sec;
        int ms = tv.tv_usec / 1000;

        struct tm temp;
        struct tm *curtime = localtime_r( &tt, &temp );
        int year = 1900 + curtime->tm_year;
        int month = curtime->tm_mon + 1;
        int day = curtime->tm_mday;
        int hour = curtime->tm_hour;
        int minute = curtime->tm_min;
        int second = curtime->tm_sec;
    
        std::stringstream oss;
        oss << std::setw(4) << std::setfill('0') << year << "-"
            << std::setw(2) << std::setfill('0') << month << "-"
            << std::setw(2) << std::setfill('0') << day << " "
            << std::setw(2) << std::setfill('0') << hour << ":"
            << std::setw(2) << std::setfill('0') << minute << ":"
            << std::setw(2) << std::setfill('0') << second << "."
            << std::setw(3) << std::setfill('0') << ms; 
        return oss.str();
    }

}

#endif

