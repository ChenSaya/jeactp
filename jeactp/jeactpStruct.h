#ifndef _JEACTPSTRUCT_H_
#define _JEACTPSTRUCT_H_

#include <map>
#include <string>
#include "threadstdqueue.h"

#include "ThostFtdcMdApi.h"

using namespace jealib;


struct SMarketData 
{
    char  m_RecvTime[30];     //接收时间
    unsigned int m_uiTickCntRcv; //接收到行情数据的时间
    CThostFtdcDepthMarketDataField oDepthMarketData;  //深度行情数据
};

struct SOrderInfo
{
    TThostFtdcOrderRefType  OrderRef;
    char  m_RecvTime[30];     //接收时间

    unsigned int m_uiTickCntRcv;           //接收到行情数据的时间
    unsigned int m_uiTickCntToWorker;      //到达worker的时间
    unsigned int m_uiTickCntSendOrder;     //发送报单的时间
    unsigned int m_uiTickCntFirstRtnOrder; //发送报单后，服务端首次返回响应的时间
    unsigned int m_uiTickCntRtnTrade;      //成交时间
};



#endif

