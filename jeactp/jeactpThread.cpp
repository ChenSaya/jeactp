#include "jeactpThread.h"
#include "simplelog.h"
#include "jeactpMdspiHandler.h"

#include <vector>
#include <string>

using namespace jealib;

CJeactpThreadMgr::CJeactpThreadMgr() : m_bIsGoingtoExit(false), m_pJeactpStorage(NULL), m_pTraderApi(NULL), m_pJeactpTrader(NULL)
{
}

/*
CJeactpThreadMgr::CJeactpThreadMgr(const CJeactpThreadMgr &oJeactpThreadMgr)
{
}
*/

CJeactpThreadMgr::~CJeactpThreadMgr()
{
    if (NULL != m_pJeactpStorage) delete m_pJeactpStorage;
    if (NULL != m_pJeactpTrader)  delete m_pJeactpTrader;
}

CJeactpThreadMgr& CJeactpThreadMgr::GetInstance()
{
    static CJeactpThreadMgr instance;
    return instance;
}

int CJeactpThreadMgr::Init(PROCESS_TYPE eProcessType, const std::vector<std::string> &oInstrumentList)
{
    int ret = 0;

    int size = oInstrumentList.size();
    for (int i = 0; i < size; ++i)
    {
        m_oInstrumentList.push_back(oInstrumentList[i]);
    }
    LOG << LOGBEG << "m_oInstrumentList.size: " << (int)m_oInstrumentList.size() << LOGEND;
    LOG << LOGBEG << "m_oInstrumentList.size: " << (int)(GetInstance().m_oInstrumentList.size()) << LOGEND;

    if ((ret = StartThread(eProcessType)) != 0)
    {
        LOGERR << LOGBEG << "StartThread fail, ret=" << ret << LOGEND;
        return ret;
    }
    return 0;
}

int CJeactpThreadMgr::StartThread(PROCESS_TYPE eProcessType)
{
    int ret = 0;
    pthread_t tid;

/*    //创建行情数据接收线程
    if ((ret = pthread_create(&tid , NULL , ThreadMdSpiProc, NULL)) != 0)
    {
        LOGERR << LOGBEG << "Can't create thread 'ThreadMdSpiProc', ret=" << ret << LOGEND;
        return ret;
    }
*/
    //根据应用类型确定是否启动存储处理线程
    //创建数据存储处理线程
    if (PROCESS_AS_RECV_DATA == eProcessType)
    {
        m_pJeactpStorage = new CJeactpStorage();

        //起若干个执行数据存储操作的线程
        for (int i = 0; i < 2; ++i)
        {
            if ((ret = pthread_create(&tid , NULL , ThreadStorageProc, NULL)) != 0)
            {
                LOGERR << LOGBEG << "Can't create thread 'ThreadStorageProc', ret=" << ret << LOGEND;
                return ret;
            }
        }

        //数据文件检查线程，当一段时间无数据输入时则关闭文件
        if ((ret = pthread_create(&tid , NULL , ThreadStorageCheckCloseFileProc, NULL)) != 0)
        {
            LOGERR << LOGBEG << "Can't create thread 'ThreadStorageCheckCloseFileProc', ret=" << ret << LOGEND;
            return ret;
        }
    }

    //根据应用类型确定是否启动模型计算及交易线程
    //创建计算worker线程
    if (PROCESS_AS_TRADER == eProcessType)
    {
        m_pTraderApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
        m_pJeactpTrader = new CJeactpTrader(m_pTraderApi);
        m_pTraderApi->RegisterSpi(m_pJeactpTrader);

        //创建交易API线程
        if ((ret = pthread_create(&tid , NULL , ThreadTraderApiProc, NULL)) != 0)
        {
            LOGERR << LOGBEG << "Can't create thread 'ThreadTraderApiProc', ret=" << ret << LOGEND;
            return ret;
        }

        //起若干个模型计算线程
        for (int i = 0; i < 1; ++i)
        {
            if ((ret = pthread_create(&tid , NULL , ThreadTraderWorkerProc, NULL)) != 0)
            {
                LOGERR << LOGBEG << "Can't create thread 'ThreadTraderWorkerProc', ret=" << ret << LOGEND;
                return ret;
            }
        }
    } 
    
    //创建行情数据接收线程
    if ((ret = pthread_create(&tid , NULL , ThreadMdSpiProc, NULL)) != 0)
    {
        LOGERR << LOGBEG << "Can't create thread 'ThreadMdSpiProc', ret=" << ret << LOGEND;
        return ret;
    }

    return 0;
}

// 行情数据接收线程入口
void * CJeactpThreadMgr::ThreadMdSpiProc(void *arg)
{
    LOG << LOGBEG << "Thread Begin..." << LOGEND;

    // 产生一个CThostFtdcMdApi实例
    CThostFtdcMdApi *pMdApi = CThostFtdcMdApi::CreateFtdcMdApi();
    // 产生一个事件处理的实例
    LOG << LOGBEG << "m_oInstrumentList.size: " << (int)(GetInstance().m_oInstrumentList.size()) << LOGEND;
    CJeactpMdspiHandler oJeactpMdspiHandler(pMdApi, GetInstance().m_oInstrumentList, GetInstance().m_pJeactpStorage, GetInstance().m_pJeactpTrader);
    // 注册一事件处理的实例
    pMdApi->RegisterSpi(&oJeactpMdspiHandler);

    // 设置交易托管系统服务的地址，可以注册多个地址备用
    //pMdApi->RegisterFront("(char*)tcp://180.168.146.187:10010"); //simnow 第一套 第一组 
    //pMdApi->RegisterFront("(char*)tcp://180.168.146.187:10011"); //simnow 第一套 第二组 
    pMdApi->RegisterFront((char*)"tcp://180.168.146.187:10031"); //simnow 第二套 
    //pMdApi->RegisterFront((char*)"tcp://180.166.64.161:41213"); //pingan qihuo
    //pMdApi->RegisterFront((char*)"tcp://140.206.102.234:41213"); //pingan qihuo
    //pUserApi->RegisterFront("tcp://180.168.146.187:10011");
    //pUserApi->RegisterFront("tcp://218.202.237.33 :10012");

    // 使客户端开始与后台服务建立连接
    pMdApi->Init();

    // 客户端等待报单操作完成
    // WaitForSingleObject(g_hEvent, INFINITE);
    // 释放API实例
    pMdApi->Release();

    LOGERR << LOGBEG << "Thread END !!!" << LOGEND;
    return 0;
}

void * CJeactpThreadMgr::ThreadStorageProc(void *arg)
{
    LOG << LOGBEG << "Thread Begin..." << LOGEND;
    while (true)
    {
        GetInstance().m_pJeactpStorage->SaveDataToFile();
    }

    LOGERR << LOGBEG << "Thread END !!!" << LOGEND;
    exit(-1);
    return 0;
}

void * CJeactpThreadMgr::ThreadStorageCheckCloseFileProc(void *arg)
{
    LOG << LOGBEG << "Thread Begin..." << LOGEND;
    while (true)
    {
        GetInstance().m_pJeactpStorage->CheckCloseFile();
        sleep(10); 
    }

    LOGERR << LOGBEG << "Thread END !!!" << LOGEND;
    exit(-1);
    return 0;
}

void * CJeactpThreadMgr::ThreadTraderWorkerProc(void *arg)
{
    LOG << LOGBEG << "Thread Begin..." << LOGEND;
    int ret = 0;
    while (true)
    {
        if ((ret = GetInstance().m_pJeactpTrader->WorderProcess()) != 0)
        {
            LOGERR << LOGBEG << "WorderProcess fail, ret=" << ret << LOGEND;
        }
    }
    LOGERR << LOGBEG << "Thread END !!!" << LOGEND;
    exit(-1);
    return NULL;
}

void * CJeactpThreadMgr::ThreadTraderApiProc(void *arg)
{
    LOG << LOGBEG << "Thread Begin..." << LOGEND;
    CThostFtdcTraderApi* pTraderApi = GetInstance().m_pTraderApi;

    // 订阅私有流
    //   THOST_TERT_RESTART:从本交易日开始重传
    //   THOST_TERT_RESUME:从上次收到的续传
    //   THOST_TERT_QUICK:只传送登录后私有流的内容 
    pTraderApi->SubscribePrivateTopic(THOST_TERT_RESUME);

    // 订阅公共流
    //   THOST_TERT_RESTART:从本交易日开始重传
    //   THOST_TERT_RESUME:从上次收到的续传
    //   THOST_TERT_QUICK:只传送登录后公共流的内容
    pTraderApi->SubscribePublicTopic(THOST_TERT_RESUME);

    // 设置交易托管系统服务的地址，可以注册多个地址备用
    //pTraderApi->RegisterFront("tcp://172.16.0.31:57205");
    //pTraderApi->RegisterFront("tcp://ctp24-front1.financial-trading-platform.com:41205");
    //pTraderApi->RegisterFront("tcp://ctp24-front2.financial-trading-platform.com:41205");
    //pTraderApi->RegisterFront("tcp://asp-sim2-front1.financial-trading-platform.com:26205");
    //pTraderApi->RegisterFront((char*)"tcp://180.168.146.187:10000"); // simnow 第一套 第一组 
    pTraderApi->RegisterFront((char*)"tcp://180.168.146.187:10030"); // simnow 第二套 

    // 使客户端开始与后台服务建立连接
    pTraderApi->Init();

    // 释放API实例
    pTraderApi->Release();

    LOGERR << LOGBEG << "Thread END !!!" << LOGEND;
    return NULL;
}

void CJeactpThreadMgr::MainThreadRun()
{
    LOG << LOGBEG << "Thread Begin..." << LOGEND;
    int ret = 0;
    while (true)
    {
        if ((ret =GetInstance().m_pJeactpTrader->CheckLogic()) != 0)
        {
            LOGERR << LOGBEG << "CheckLogic() fail, ret=" << ret << LOGEND;
        }
        sleep(1);
    }

    LOGERR << LOGBEG << "Thread END !!!" << LOGEND;
}


