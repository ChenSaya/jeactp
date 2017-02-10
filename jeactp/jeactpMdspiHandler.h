#ifndef _JEACTPMDSPIHANDLER_H_
#define _JEACTPMDSPIHANDLER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ThostFtdcMdApi.h"
#include "jeactpStruct.h"
#include "jeactpStorage.h"
#include "jeactpTrader.h"

class CJeactpMdspiHandler : public CThostFtdcMdSpi
{
public:
    // 构造函数，需要一个有效的指向CThostFtdcMdApi实例的指针
    CJeactpMdspiHandler(CThostFtdcMdApi *pMdApi, const std::vector<std::string> &oInstrumentList, CJeactpStorage* pJeactpStorage, CJeactpTrader* pJeactpTrader);
    ~CJeactpMdspiHandler();

    // 当客户端与交易托管系统建立起通信连接，客户端需要进行登录
    virtual void OnFrontConnected();

    // 当客户端与交易托管系统通信连接断开时，该方法被调用
    virtual void OnFrontDisconnected(int nReason);

    // 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 行情应答
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

    // 针对用户请求的出错通知
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

private:
    CThostFtdcMdApi*         m_pMdApi;           // 指向CThostFtdcMdApi实例的指针
    std::vector<std::string> m_oInstrumentList;  //接收数据的合约列表
    CJeactpStorage*          m_pJeactpStorage; 
    CJeactpTrader *          m_pJeactpTrader;

};

#endif

