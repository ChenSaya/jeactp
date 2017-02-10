#include "jeactpMdspiHandler.h"
#include "simplelog.h"

#include "util.h"

CJeactpMdspiHandler::CJeactpMdspiHandler(CThostFtdcMdApi *pMdApi, const std::vector<std::string> &oInstrumentList, CJeactpStorage* pJeactpStorage, CJeactpTrader* pJeactpTrader) 
    : m_pMdApi(pMdApi), m_pJeactpStorage(pJeactpStorage), m_pJeactpTrader(pJeactpTrader)
{
    int size = oInstrumentList.size();
    for (int i = 0; i < size; ++i)
    {
        m_oInstrumentList.push_back(oInstrumentList[i]);
    }
}

CJeactpMdspiHandler:: ~CJeactpMdspiHandler() 
{
}

// 当客户端与交易托管系统建立起通信连接，客户端需要进行登录
void CJeactpMdspiHandler::OnFrontConnected()
{
    LOG << LOGBEG << "begin..."  << LOGEND;
    CThostFtdcReqUserLoginField reqUserLogin;
    
    // 会员代码
    //TThostFtdcBrokerIDType g_chBrokerID;
    // 交易用户代码
    //TThostFtdcUserIDType g_chUserID;

    // get BrokerID
    //printf("BrokerID:");
    //scanf("%s", (char*)&g_chBrokerID);
    strcpy(reqUserLogin.BrokerID, "123");

    // get userid
    //printf("userid:");
    //scanf("%s", (char*)&g_chUserID);
    strcpy(reqUserLogin.UserID, "123");

    // get password
    //printf("password:");
    //scanf("%s", (char*)&reqUserLogin.Password);
    // 发出登陆请求
    m_pMdApi->ReqUserLogin(&reqUserLogin, 0);

    LOG << LOGBEG << "end."  << LOGEND;
}

// 当客户端与交易托管系统通信连接断开时，该方法被调用
void CJeactpMdspiHandler::OnFrontDisconnected(int nReason)
{
    // 当发生这个情况后，API会自动重新连接，客户端可不做处理
    LOG << LOGBEG << "begin..."  << LOGEND;
    LOG << LOGBEG << "end."  << LOGEND;
}


// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
void CJeactpMdspiHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG << LOGBEG << "begin..."  << LOGEND;
    LOG << LOGBEG  
        << " ErrorCode=" << pRspInfo->ErrorID  
        << " ErrorMsg="  << pRspInfo->ErrorMsg  
        << " RequestID=" << nRequestID  
        << " Chain="     << bIsLast  
        << LOGEND;

    if (pRspInfo->ErrorID != 0) 
    {
        // 端登失败，客户端需进行错误处理
        LOGERR << LOGBEG << "Failed to login," 
               << " errorcode=" << pRspInfo->ErrorID 
               << " errormsg="  << pRspInfo->ErrorMsg 
               << " requestid=" << nRequestID
               << " chain="     << bIsLast
               << LOGEND;
        exit(-1);
    }

    // 端登成功
    // 订阅行情
    char* Instrumnet[m_oInstrumentList.size()];
    int size = sizeof(Instrumnet)/sizeof(Instrumnet[0]);
    memset(&Instrumnet[0], 0, sizeof(Instrumnet));
    LOG << LOGBEG << "!!INSTRUMENT SIZE: " << size << LOGEND;
    for (int i = 0; i < size; ++i)
    {
        //LOG << LOGBEG << "    > " << InsList[i]  << LOGEND;
        char* p = new char[100];
        memset(p, 0, sizeof(100));
        Instrumnet[i] = p;
        memcpy(p, m_oInstrumentList[i].c_str(), m_oInstrumentList[i].size() + 1);
    }
    for (int i = 0; i < size; ++i)
    {
        LOG << LOGBEG << "    :: " << Instrumnet[i]  << LOGEND;
    }
    //char * Instrumnet2[]={"cu1608", "cu1609", "PTA1608"};
    m_pMdApi->SubscribeMarketData (Instrumnet, size);
    for (int i = 0; i < size; ++i)
    {
        //LOG << LOGBEG << "    : " << Instrumnet[i]  << LOGEND;
        delete Instrumnet[i];
    }
    
    //或退订行情
    //m_pUserApi->UnSubscribeMarketData (Instrumnet,2);
    LOG << LOGBEG << "end."  << LOGEND;
}

// 行情应答
void CJeactpMdspiHandler::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    LOG << LOGBEG << "begin..."  << LOGEND;

    SMarketData oMarketData;
    memset(&oMarketData, 0, sizeof(oMarketData));
    oMarketData.m_uiTickCntRcv = jealib::GetTickCount();
    //std::string strTime = GetCurrentTimeWithMS();
    //memset(&oMarketData.m_RecvTime[0], 0, sizeof(oMarketData.m_RecvTime));
    //memcpy(&oMarketData.m_RecvTime[0], strTime.c_str(), strTime.size());
    memcpy(&oMarketData.oDepthMarketData, pDepthMarketData, sizeof(oMarketData.oDepthMarketData));

    //下面将数据发往存储处理线程或模型计算线程。一般来说这两者是二选一。
    if (NULL != m_pJeactpStorage) m_pJeactpStorage->SendToStorageThreadQueue(oMarketData);
    if (NULL != m_pJeactpTrader)  m_pJeactpTrader->SendToTraderThreadQueue(oMarketData);

    LOG << LOGBEG << "end."  << LOGEND;
} 

// 针对用户请求的出错通知
void CJeactpMdspiHandler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    LOGERR << LOGBEG << "begin..." << LOGEND;
    LOGERR << LOGBEG  
           << " ErrorCode=" << pRspInfo->ErrorID
           << " ErrorMsg="  << pRspInfo->ErrorMsg
           << " RequestID=" << nRequestID
           << " Chain="     << bIsLast 
           << LOGEND;

    // 客户端需进行错误处理
    // todo...

    LOGERR << LOGBEG << "end."  << LOGEND;
}





