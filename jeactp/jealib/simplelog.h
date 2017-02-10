#ifndef _SIMPLELOG_H_
#define _SIMPLELOG_H_

#include <cstdio>
#include <string.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "threadlock.h"

#define LOG jealib::CLog::GetDefault()
#define LOGERR jealib::CLog::GetDefault(jealib::CLog::LOG_LEVEL_ERROR)
#define LOGBEG jealib::CLog::GetLogBeg(__FILE__, __LINE__, __func__)
#define LOGEND jealib::CLog::GetLogEnd()

namespace jealib
{

struct SLogBeg
{
    const char*  pFileName;
    unsigned int line;
    const char*  pFuncName;
};

struct SLogEnd
{
};

class CLog
{
    enum
    {
        DAY_SECONDS = 60 * 60 * 24,
    };

public:
    enum
    {
        LOG_LEVEL_INFO  = 1,
        LOG_LEVEL_ERROR = 2,

    };

public:
    CLog(const std::string& filename, unsigned int uiMaxFileLen, unsigned int uiDays, int nBufsize);
    ~CLog();
    static CLog& GetDefault(int iLevel = CLog::LOG_LEVEL_INFO);
    static const SLogBeg& GetLogBeg(const char* pFileName, unsigned int line, const char* pFuncName);
    static const SLogEnd& GetLogEnd();
    void SetLogFileName(std::string filename);
    
    bool Write(void const* pbuf, int nSize, bool bForce);
    CLog& operator << (const SLogBeg &logbeg);
    CLog& operator << (const SLogEnd &logend);
    //CLog& operator << (const char* &d);
    //CLog& operator << (char* &d);
    CLog& operator << (const char &d);
    CLog& operator << (const char d[]);
    CLog& operator << (int d);
    CLog& operator << (unsigned int d);
    CLog& operator << (long d);
    CLog& operator << (long long d);
    CLog& operator << (float d);
    CLog& operator << (double d);
    CLog& operator << (bool d);
    CLog& operator << (const std::string &d);

private:
    bool DoWrite(void const * pbuf, int nSize, bool bForce);
private:
    FILE * m_pFile;
    int m_iCurDay;
    std::string m_oFilePre;
    int m_nPos;
    const int MAX_BUFSIZE;
    unsigned char * m_oBuf;//[MAX_BUFSIZE];
    std::stringstream m_oStream;
    unsigned int m_uiLen ;
    unsigned int m_uiMaxLen ;
    unsigned int m_uiDays ;

    CMutexLock m_oLock;
    int m_iLogLevel;

};

}

#endif

