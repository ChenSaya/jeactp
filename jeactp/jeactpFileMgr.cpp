#include "jeactpFileMgr.h"
#include "simplelog.h"
#include <stdlib.h>

std::string GetPathFromPathFileName(std::string strPathFileName)  
{
    std::string strPath = "./";
    std::string::size_type iPos = strPathFileName.rfind('/') ;

    if (iPos != std::string::npos)
    {
        strPath = strPathFileName.substr(0, iPos + 1) ;
    }
    return strPath;
}

bool MakeDirectory(std::string strPath)
{
    if (!access(strPath.c_str(),0)) {
        return true;
    }
	
    std::string strCmd;
    strCmd = "mkdir -p " + strPath;
    system(strCmd.c_str());

    if (!access(strPath.c_str(),0))
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**********************************************************/

CDataFile::CDataFile(const std::string &strFileName)
{
    m_strFileName = strFileName;
    OpenFile();
}

CDataFile::~CDataFile()
{
    CloseFile();
}

CDataFile& CDataFile::operator << (const char* pcStr)
{
    LOG << LOGBEG << "pcStr: " << pcStr  << LOGEND;
    m_bHasInputFlag = true;
    m_oFile << pcStr;
    return *this;
}

void CDataFile::ResetInputFlag()
{
    m_oFile.flush();
    m_bHasInputFlag = false;
}

bool CDataFile::GetInputFlag()
{
    return m_bHasInputFlag;
}

int CDataFile::OpenFile()
{
    if (m_oFile.is_open()) 
    {
        return 0;
    }
 
    LOG << LOGBEG << "Open file: " << m_strFileName << LOGEND;
    std::string strPath = GetPathFromPathFileName(m_strFileName);
    MakeDirectory(strPath);
    m_oFile.open(m_strFileName.c_str(), std::ios_base::out | std::ios_base::app | std::ios_base::binary);
    if (m_oFile.is_open())
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int CDataFile::CloseFile()
{
    if (m_oFile.is_open())
    {
        LOG << LOGBEG << "Close file: " << m_strFileName << LOGEND;
        m_oFile.close();
    }
    return 0;
}

/**********************************************************/

CJeactvFileMgr::CJeactvFileMgr()
{

}

CJeactvFileMgr::~CJeactvFileMgr()
{

}
 
int CJeactvFileMgr::Write(const std::string & strFileName, const std::string & strData)
{
    CAutoMutexLock autoLock(m_lock);
    std::map<std::string, CDataFile*>::iterator itr = m_oFileMap.find(strFileName);
    CDataFile* pDataFile = NULL;
    if (itr == m_oFileMap.end())
    {
        pDataFile = new CDataFile(strFileName);
        m_oFileMap[strFileName] = pDataFile;
    }
    else
    {
        pDataFile = itr->second;
    }
	
    *pDataFile << strData.c_str();
	
    return 0;
}

void CJeactvFileMgr::CheckCloseFile()
{
    LOG << LOGBEG << "begin ..." << LOGEND;
    CAutoMutexLock autoLock(m_lock);
    std::map<std::string, CDataFile*>::iterator itr = m_oFileMap.begin();
    for (; itr != m_oFileMap.end();)
    {
        CDataFile* pDataFile = itr->second;
        if (!pDataFile->GetInputFlag())
        {
            m_oFileMap.erase(itr++);
            delete pDataFile;
        }
        else
        {
            pDataFile->ResetInputFlag();
            ++itr;
        }
    }
}

