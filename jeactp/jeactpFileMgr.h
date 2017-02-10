#ifndef _JEACTPFILEMGR_H_
#define _JEACTPFILEMGR_H_

#include <string>
#include <map>
#include <fstream>
#include "threadlock.h"

class CDataFile
{
public:
    CDataFile(const std::string &strFileName);
    ~CDataFile();

    CDataFile& operator << (const char* pcStr);

    void ResetInputFlag();
    bool GetInputFlag();

private:
    int OpenFile();
    int CloseFile();

    std::string   m_strFileName;
    std::ofstream m_oFile;
    bool          m_bHasInputFlag;  //标记是否有数据输入，若无数据输入，过一段时间后可将文件会被关闭
};

/**********************************************************/

class CJeactvFileMgr
{
public:
    CJeactvFileMgr();
    ~CJeactvFileMgr();

    int Write(const std::string & strFileName, const std::string & strData);
    void CheckCloseFile();
    
    CMutexLock m_lock; 

private:
    std::map<std::string, CDataFile*> m_oFileMap;

};

#endif

