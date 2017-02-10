#ifndef _JEACTPTRADER_H_
#define _JEACTPTRADER_H_

#include <string.h>
#include "threadstdqueue.h" 
#include "ThostFtdcTraderApi.h"
#include "jeactpStruct.h"
#include "jeactpTraderCache.h"

extern TThostFtdcBrokerIDType g_chBrokerID;  // 会员代码
extern TThostFtdcUserIDType   g_chUserID;    // 交易用户代码
extern TThostFtdcPasswordType g_chPassword;  // 密码

enum MODEL_RESULT_TYPE
{
    MODEL_RESULT_BUY    = 1,  // 买入
    MODEL_RESULT_SELL   ,     // 卖出
    MODEL_RESULT_IGNORE ,     // 忽略，不买也不卖，保持不变
    MODEL_RESULT_FAIL   ,     // 计算失败

};

// Trader状态
struct TraderState
{
public:    
    TraderState();
    void SetLoginTime();
    void SetSendReqQrySettlementInfoConfirmTime();
    void SetSendReqQrySettlementInfoTime();

    unsigned int program_start_time;

    bool         is_logined;  // 是否登录成功
    unsigned int login_time;  // 登录成功时间(TickCount)

    bool         is_send_ReqQrySettlementInfoConfirm;    // 是否已发送查询结算信息确认
    unsigned int send_ReqQrySettlementInfoConfirm_time;  // 发送查询结算信息确认的时间，若n秒后无返回，则未确认，需发起确认 

    bool         has_sent_ReqQrySettlementInfo;  // 是否已发送ReqQrySettlementInfo
    unsigned int time_sent_ReqQrySettlementInfo; // 发送ReqQrySettlementInfo时间

    bool         has_confirmed_SettlementInfo;  // 是否已确认 (确认后可进入交易) 

private:
    TraderState(const TraderState &o);
};

class CJeactpTrader : public CThostFtdcTraderSpi
{
public:
    CJeactpTrader(CThostFtdcTraderApi *pUserApi);
    ~CJeactpTrader();

    /***********************************************************/
    int SendToTraderThreadQueue(const SMarketData &oMarketData); //发送数据到处理线程(即入队处理）

    int WorderProcess();  //模型计算和交易处理过程

    int CheckLogic();  // 一般检查逻辑，定时循环检查，根据检查结果启动相关操作（该函数由循环线程驱动）

    /***********************************************************/
    // 当客户端与交易托管系统建立起通信连接，客户端需要进行登录
    virtual void OnFrontConnected();

    // 当客户端与交易托管系统通信连接断开时，该方法被调用
    virtual void OnFrontDisconnected(int nReason);

    // 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 报单录入应答
    virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 报单回报 (交易所回报, 委托回报)
    virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);
 
    // 成交回报
    virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

    // 报单录入错误回报。由交易托管系统主动通知客户端，该方法会被调用。
    virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

    // 报单操作错误回报
    virtual void OnErrRtnOrderAction (CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

    // 针对用户请求的出错通知
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 查询结算确认响应 
    virtual void OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 请求查询投资者结算结果响应 
    virtual void OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    // 投资者结算结果确认应答
    virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

private:
    CJeactpTrader();
    CJeactpTrader(const CJeactpTrader &JeactpTrader);
    int GetMarketDataFromQueue(SMarketData &oMarketData); // 从队列取行情数据
    MODEL_RESULT_TYPE ModelCalculation(HISTORY_DATA_OUTPUT_TYPE &oLastHistoryData);  // 模型计算

    // 发送报单请求
    int SendOrder(const char* pcInstrumentID, TThostFtdcDirectionType Direction, 
        TThostFtdcPriceType LimitPrice, TThostFtdcVolumeType VolumeTotalOriginal,
        TThostFtdcOffsetFlagType OffsetFlag); 

    std::string GetCtpReqResultCodeMsg(int code);  // 取请求函数返回值对应的错误描述 
    int My_ReqQrySettlementInfoConfirm(); // 查询结算信息确认
    int My_ReqQrySettlementInfo(); // 请求查询投资者结算结果
    int My_ReqSettlementInfoConfirm();  // 投资者结算结果确认

    void IncMaxOrderRe();
    SOrderInfo* GetHistoryOrder(std::string strOrderRef);

    CThostFtdcTraderApi     *m_pUserApi;  // 指向CThostFtdcMduserApi实例的指针
    jealib::CThreadStdQueue m_oDataQueue;  // 从数据接收线程发送过来的原始数据
    CJeactpTraderCache      m_oJeactpTraderCache; //历史数据缓存

    TThostFtdcFrontIDType   m_iFrontID;      // 前置编号
    TThostFtdcSessionIDType m_iSessionID;    // 会话编号
    TThostFtdcOrderRefType  m_chMaxOrderRef; // 最大报单引用

    int                     m_nRequestID;

    TraderState             m_oTraderState;  // Trader状态
    std::map<std::string, SOrderInfo> m_oHistoryOrder;  //历史报单信息  
};

#endif

