#include "simplelog.h"

#include <time.h>
#include <sys/time.h>

namespace jealib
{

CLog::CLog(const std::string& filename, unsigned int uiMaxFileLen = 200 * 1024 * 1024, unsigned int uiDays = 10, int nBufsize = 10 * 1024)
    : m_pFile(0), m_iCurDay(-1), m_oFilePre(filename), m_nPos(0),
    MAX_BUFSIZE(nBufsize), m_uiLen(0), m_uiMaxLen(uiMaxFileLen), m_uiDays(uiDays), m_iLogLevel(0)
{
    m_oBuf = new unsigned char[nBufsize];
}

CLog::~CLog()
{
    delete []m_oBuf;
    if (m_pFile)
    {
        fclose(m_pFile);
    }
}

void CLog::SetLogFileName(std::string filename)
{
    m_oFilePre = filename;
}

CLog& CLog::GetDefault(int iLevel)
{
    static CLog log("/tmp/mylog_default");
    log.m_iLogLevel = iLevel;
    return log;
}

const SLogBeg& CLog::GetLogBeg(const char* pFileName, unsigned int line, const char* pFuncName)
{
    static SLogBeg logbeg;
    logbeg.pFileName = pFileName;
    logbeg.line      = line;
    logbeg.pFuncName = pFuncName; 
    return logbeg; 
}

const SLogEnd& CLog::GetLogEnd()
{
    static SLogEnd logend;
    return logend; 
}

bool CLog::Write(void const* pbuf, int nSize, bool bForce = false)
{
    if (m_oFilePre.empty())
    {
        return false ;
    } 

    bool ret = false;
    time_t now = time(0);
    tm * pTime = localtime(&now);
    if (m_iCurDay != pTime->tm_mday)
    {
        m_uiLen = 0;
        DoWrite(pbuf, 0, true);
        m_iCurDay = pTime->tm_mday;
        if (m_pFile) 
        {
            fclose(m_pFile);
            m_pFile = 0;
        }
        std::stringstream oss;
        oss << m_oFilePre << "." << pTime->tm_year + 1900
            << "-" << std::setw(2) << std::setfill('0') << pTime->tm_mon + 1
            << "-" << std::setw(2) << std::setfill('0') << pTime->tm_mday
            << ".log";
        m_pFile = fopen(oss.str().c_str(), "ab");
        if (m_pFile)
        {
            fseek(m_pFile, 0, SEEK_END);
        }
        ret = DoWrite(pbuf, nSize, bForce);


        // 删除以前的日记文件
        time_t iAgoDay = now - m_uiDays * DAY_SECONDS ;
        tm * pAgoTime = localtime(&iAgoDay);
        std::stringstream oss_agoname, oss_agobackname ;
        oss_agobackname << m_oFilePre << "_back" << "." << pAgoTime->tm_year + 1900
            << "-" << std::setw(2) << std::setfill('0') << pAgoTime->tm_mon + 1
            << "-" << std::setw(2) << std::setfill('0') << pAgoTime->tm_mday
            << ".log";
        unlink(oss_agobackname.str().c_str());

        oss_agoname << m_oFilePre << "." << pAgoTime->tm_year + 1900
            << "-" << std::setw(2) << std::setfill('0') << pAgoTime->tm_mon + 1
            << "-" << std::setw(2) << std::setfill('0') << pAgoTime->tm_mday
            << ".log";
         unlink(oss_agoname.str().c_str());
    }
    else
    {
        ret = DoWrite(pbuf, nSize, bForce);
    }
    m_uiLen += nSize ;
    if (ret)
    {
        if (m_uiLen >= m_uiMaxLen)
        {
            m_uiLen = 0;
            if (m_pFile) 
            {
                fclose(m_pFile);
                m_pFile = 0;
            }
            std::stringstream oss_backname, oss_curname;
            oss_backname << m_oFilePre << "_back" << "." << pTime->tm_year + 1900
                << "-" << std::setw(2) << std::setfill('0') << pTime->tm_mon + 1
                << "-" << std::setw(2) << std::setfill('0') << pTime->tm_mday
                << ".log";

            oss_curname << m_oFilePre << "." << pTime->tm_year + 1900
                << "-" << std::setw(2) << std::setfill('0') << pTime->tm_mon + 1
                << "-" << std::setw(2) << std::setfill('0') << pTime->tm_mday
                << ".log";

            // 删除备份文件
            unlink(oss_backname.str().c_str());

            // 改名
            rename(oss_curname.str().c_str(), oss_backname.str().c_str());

            // 重新打开新的文件
            m_pFile = fopen(oss_curname.str().c_str(), "ab");
        }
    }
    return ret;
}

bool CLog::DoWrite(void const * pbuf, int nSize, bool bForce)
{
    if (m_pFile && nSize <= MAX_BUFSIZE)
    {
        if (bForce)
        {
            if (m_nPos > 0)
            {
                fwrite(m_oBuf, 1, m_nPos, m_pFile);
                m_nPos = 0 ;
            }
            if (nSize > 0)
            {	
                if (fwrite(pbuf, 1, nSize, m_pFile) > 0)
                {
                    //int aaTest = 0 ;
                }		
            }
            fflush(m_pFile);
        }
        else
        {
            if (nSize + m_nPos >= MAX_BUFSIZE)
            {
                fwrite(m_oBuf, 1, m_nPos, m_pFile);
                fflush(m_pFile);
                memcpy(m_oBuf, pbuf, nSize);
                m_nPos = nSize;
            }
            else
            {
                memcpy(m_oBuf + m_nPos, pbuf, nSize);
                m_nPos += nSize;
            }
        }
        return true;
    }
    return false;
}

CLog& CLog::operator << (const SLogBeg &logbeg)
{
    m_oLock.Lock() ;

    m_oStream.str("");

/*
    time_t lTime = time(0);
    tm * pTime = localtime(&lTime);
    m_oStream << "<" << std::setw(2) << std::setfill('0') << pTime->tm_mday << " " 
        << std::setw(2) << std::setfill('0') << pTime->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << pTime->tm_min << ":"
        << std::setw(2) << std::setfill('0') << pTime->tm_sec << ">";		
*/

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

    m_oStream << "<" 
        << std::setw(4) << std::setfill('0') << year << "-"
        << std::setw(2) << std::setfill('0') << month << "-"
        << std::setw(2) << std::setfill('0') << day << " "
        << std::setw(2) << std::setfill('0') << hour << ":"
        << std::setw(2) << std::setfill('0') << minute << ":"
        << std::setw(2) << std::setfill('0') << second << "."
        << std::setw(3) << std::setfill('0') << ms << "> "
        ;		

    m_oStream << "<" ;
    if (LOG_LEVEL_INFO == m_iLogLevel)
    {
        m_oStream << "Info";
    }
    else if (LOG_LEVEL_ERROR == m_iLogLevel)
    {
        m_oStream << "Error";
    }
    m_oStream << "> " ;

    m_oStream << "<" << logbeg.pFileName << ":" << logbeg.line << ":" << logbeg.pFuncName << "> ";

    return *this ;
}

CLog& CLog::operator << (const SLogEnd &logend)
{
    m_oStream << "\n" ;
    Write(m_oStream.str().c_str(), m_oStream.str().length(), true);
    m_oStream.str("");
    m_oLock.Unlock() ;
    return *this ;
}
/*
CLog& CLog::operator << (const char* &d)
{
    m_oStream << d ;
    return *this ;
}
*/
/*
CLog& CLog::operator << (char* &d)
{
    m_oStream << d ;
    return *this ;
}
*/
CLog& CLog::operator << (const char &d)
{
    m_oStream << d ;
    return *this ;
}

CLog& CLog::operator << (const char d[])
{
    m_oStream << d ;
    return *this ;
}

CLog& CLog::operator << (int d)
{
    m_oStream << d ;
    return *this ;
}

CLog& CLog::operator << (unsigned int d)
{
    m_oStream << d ;
    return *this ;
}

CLog& CLog::operator << (long d)
{
    m_oStream << d ;
    return *this ;
}

CLog& CLog::operator << (long long d)
{
    m_oStream << d ;
    return *this ;
}

CLog& CLog::operator << (float d)
{
    m_oStream << d ;
    return *this ;
}

CLog& CLog::operator << (double d)
{
    m_oStream << d ;
    return *this ;
}

CLog& CLog::operator << (bool d)
{
    if (d)
    {
        m_oStream << "true" ;
    }
    else
    {
        m_oStream << "false" ;
    }
    return *this ;
}

CLog& CLog::operator << (const std::string &d)
{
    m_oStream << d ;
    return *this ;
}

}


