#ifndef _JEACTPSTORAGE_H_
#define _JEACTPSTORAGE_H_

#include "jeactpStruct.h"
#include "jeactpFileMgr.h"

class CJeactpStorage
{
public:
    CJeactpStorage();
    ~CJeactpStorage();

    int SendToStorageThreadQueue(const SMarketData &oMarketData); //发送数据到存储处理线程(即入队处理）

    int SaveDataToFile();
    void CheckCloseFile();

private:
    unsigned int GetFileMgrListIndex(const std::string &strFileName);

    std::string FormatOutputDataLine(const SMarketData &oMarketData);

    CThreadStdQueue m_oDataQueue; //发送到存储数据线程的数据队列
    std::vector<CJeactvFileMgr*> m_fileMgrList;
};

#endif
