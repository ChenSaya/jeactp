#ifndef _JEACTPTHREAD_H_
#define _JEACTPTHREAD_H_

#include <map>
#include <string>
#include "jeactpStruct.h"
#include "jeactpStorage.h"
#include "jeactpTrader.h"

class CJeactpThreadMgr
{
public:
    enum PROCESS_TYPE{
        PROCESS_AS_RECV_DATA = 1,
        PROCESS_AS_TRADER
    };

public:
    static CJeactpThreadMgr& GetInstance();
    ~CJeactpThreadMgr();
    int Init(PROCESS_TYPE eProcessType, const std::vector<std::string> &oInstrumentList);
    
    void MainThreadRun(); //主线程可调用该函数进行处理

private:
    CJeactpThreadMgr();
    CJeactpThreadMgr(const CJeactpThreadMgr &oJeactpThreadMgr);
    int StartThread(PROCESS_TYPE eProcessType);

    static void * ThreadMdSpiProc(void *arg);  // 行情数据接收线程入口
    static void * ThreadStorageProc(void *arg); // 数据存储线程入口
    static void * ThreadStorageCheckCloseFileProc(void *arg); //检查数据文件的线程
    static void * ThreadTraderWorkerProc(void *arg); // 数据计算worker线程入口
    static void * ThreadTraderApiProc(void *arg); // 交易处理线程入口

    std::vector<std::string> m_oInstrumentList;  //接收数据的合约列表
    bool                     m_bIsGoingtoExit;
    CJeactpStorage *         m_pJeactpStorage;
    CThostFtdcTraderApi *    m_pTraderApi;
    CJeactpTrader *          m_pJeactpTrader;
};

#endif

