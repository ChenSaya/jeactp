#include "jeactpTrader.h"

#include <stdio.h>
#include <stdlib.h>
#include "simplelog.h"
#include "stringconv.h"
#include "util.h"

TThostFtdcBrokerIDType g_chBrokerID;  // 会员代码
TThostFtdcUserIDType   g_chUserID;    // 交易用户代码
TThostFtdcPasswordType g_chPassword;  // 密码

TraderState::TraderState()
    : program_start_time(jealib::GetTickCount())
    , is_logined(false), login_time(0)
    , is_send_ReqQrySettlementInfoConfirm(false), send_ReqQrySettlementInfoConfirm_time(0)
    , has_sent_ReqQrySettlementInfo(false), time_sent_ReqQrySettlementInfo(0)
    , has_confirmed_SettlementInfo(false) 
{
}

void TraderState::SetLoginTime()
{
    is_logined = true;
    login_time = jealib::GetTickCount();
}

void TraderState::SetSendReqQrySettlementInfoConfirmTime()
{
    is_send_ReqQrySettlementInfoConfirm = true;
    send_ReqQrySettlementInfoConfirm_time = jealib::GetTickCount();
}

void TraderState::SetSendReqQrySettlementInfoTime()
{
    has_sent_ReqQrySettlementInfo = true;
    time_sent_ReqQrySettlementInfo = jealib::GetTickCount(); 
}

//////////////////////////////////////////////////////////////////////////

CJeactpTrader::CJeactpTrader(CThostFtdcTraderApi *pUserApi) 
    : m_pUserApi(pUserApi), m_iFrontID(0), m_iSessionID(0), m_nRequestID(1)
{
    memset(&m_chMaxOrderRef[0], sizeof(m_chMaxOrderRef), 0);
}

CJeactpTrader::~CJeactpTrader() 
{
}

int CJeactpTrader::SendToTraderThreadQueue(const SMarketData &oMarketData)
{
    std::vector<char> oBuffer(sizeof(oMarketData));
    memcpy(&oBuffer[0], &oMarketData, sizeof(oMarketData));

    m_oDataQueue.Enqueue(oBuffer);
    LOG << LOGBEG << "send finished."  << LOGEND;

    return 0;
}

//模型计算和交易处理过程
int CJeactpTrader::WorderProcess()
{
    LOG << LOGBEG << "begin..."  << LOGEND;
    int ret = 0;

    // 从队列取行情数据
    SMarketData oMarketData;
    if ((ret = GetMarketDataFromQueue(oMarketData)) != 0)
    {
        LOGERR << LOGBEG << "GetMarketDataFromQueue fail, ret=" << ret << LOGEND;
        return ret;
    }

    // 将行情数据保存到缓存并取得最新数据集
    HISTORY_DATA_OUTPUT_TYPE oLastHistoryData;
    if ((ret = m_oJeactpTraderCache.SaveAndGetModelNeededData(oMarketData, oLastHistoryData)) != 0)
    {
        LOGERR << LOGBEG << "SaveAndGetModelNeededData fail, ret=" << ret << LOGEND;
        return ret;
    }

    if (!m_oTraderState.has_confirmed_SettlementInfo)
    {
        // 交易未就绪
        LOGERR << LOGBEG << "!! Trading is NOt ready !!" << LOGEND;
        return -1;
    }
    

    const char* pcInstrumentID = oMarketData.oDepthMarketData.InstrumentID;
    TThostFtdcPriceType LimitPrice = oMarketData.oDepthMarketData.AskPrice1;

    SOrderInfo orderInfo;
    orderInfo.OrderRef[0] = 0;
    strcpy(orderInfo.m_RecvTime, oMarketData.m_RecvTime);
    orderInfo.m_uiTickCntRcv = oMarketData.m_uiTickCntRcv;
    orderInfo.m_uiTickCntToWorker = jealib::GetTickCount();
    orderInfo.m_uiTickCntSendOrder = 0;
    orderInfo.m_uiTickCntFirstRtnOrder = 0;
    orderInfo.m_uiTickCntRtnTrade = 0;

    // 模型计算，根据模型结果做对应的交易操作
    MODEL_RESULT_TYPE modelResult = ModelCalculation(oLastHistoryData);
    switch (modelResult)
    {
        case MODEL_RESULT_BUY:
            LOG << LOGBEG << "ModelResult: MODEL_RESULT_BUY" << LOGEND;

            if ((ret = CJeactpTrader::SendOrder(pcInstrumentID, THOST_FTDC_D_Buy, LimitPrice, 1, THOST_FTDC_OF_Open)) != 0)
            {
                LOGERR << LOGBEG << "SendOrder (buy) fail. ret=" << ret << LOGEND;
            }
            orderInfo.m_uiTickCntSendOrder = jealib::GetTickCount();

            break;
        case MODEL_RESULT_SELL:
            LOG << LOGBEG << "ModelResult: MODEL_RESULT_SELL" << LOGEND;

            if ((ret = CJeactpTrader::SendOrder(pcInstrumentID, THOST_FTDC_D_Sell, LimitPrice, 1, THOST_FTDC_OF_CloseToday)) != 0)
            {
                LOGERR << LOGBEG << "SendOrder (sell) fail. ret=" << ret << LOGEND;
            }
            orderInfo.m_uiTickCntSendOrder = jealib::GetTickCount();

            break;
        case MODEL_RESULT_IGNORE:
            LOG << LOGBEG << "ModelResult: MODEL_RESULT_IGNORE" << LOGEND;
            break;
        case MODEL_RESULT_FAIL:
            LOG << LOGBEG << "ModelResult: MODEL_RESULT_FAIL" << LOGEND;
            break;
        default:
            LOG << LOGBEG << "Unknown Model Result, modelResult=" << modelResult << LOGEND;
            break;
    }
    
    if (MODEL_RESULT_BUY == modelResult || MODEL_RESULT_SELL == modelResult)
    {
        m_oHistoryOrder.insert(std::make_pair(m_chMaxOrderRef, orderInfo));
        SOrderInfo* pOrderInfo = GetHistoryOrder(m_chMaxOrderRef);
        if (NULL != pOrderInfo)
        {
            memcpy(pOrderInfo, &orderInfo, sizeof(orderInfo));
        }
        else
        {
            LOGERR << LOGBEG << "Insert HistoryOrder fail!!" << LOGEND;
        }
    }

    LOG << LOGBEG << "end."  << LOGEND;
    return ret;
}

// 一般检查逻辑，定时循环检查，根据检查结果启动相关操作（该函数由循环线程驱动）
int CJeactpTrader::CheckLogic()
{
    LOG << LOGBEG << "begin..."  << LOGEND;

    if (jealib::GetCostTime(jealib::GetTickCount(), m_oTraderState.program_start_time) > 24 * 3600 * 1000)
    {
        LOG << LOGBEG << "Logined for a long time (>=24hours), exit..."  << LOGEND;
        exit(-1);
    }

    if (m_oTraderState.is_logined 
        && !m_oTraderState.has_confirmed_SettlementInfo    // 未确认
        && !m_oTraderState.has_sent_ReqQrySettlementInfo   // 未发送过查询

        && m_oTraderState.is_send_ReqQrySettlementInfoConfirm 
        && jealib::GetCostTime(jealib::GetTickCount(), m_oTraderState.send_ReqQrySettlementInfoConfirm_time) >= 2 * 1000  // 发送查询结算信息确认2秒后
        )
    {
        My_ReqQrySettlementInfo();
    } 

    LOG << LOGBEG << "end."  << LOGEND;
    return 0;
}

int CJeactpTrader::GetMarketDataFromQueue(SMarketData &oMarketData)
{
    LOG << LOGBEG << "begin..."  << LOGEND;
    std::vector<char> oBuffer;
    int ret = 0;
    if ((ret = m_oDataQueue.Dequeue(oBuffer)) != 0)
    {
        LOGERR << LOGBEG << "Dequeue fail."  << LOGEND;
        return ret;
    }

    if (oBuffer.size() != sizeof(oMarketData))
    {
        LOGERR << LOGBEG << "Buffer size error. oBuffer.size=" << (int)oBuffer.size() << " expected: " << (int)sizeof(oMarketData) << LOGEND;
        return -1;
    }
    memcpy(&oMarketData, &oBuffer[0], sizeof(oMarketData));

    LOG << LOGBEG << "end."  << LOGEND;
    return ret;
}

// 当客户端与交易托管系统建立起通信连接，客户端需要进行登录
void CJeactpTrader::OnFrontConnected()
{
    LOG << LOGBEG << "begin..." << LOGEND;
    CThostFtdcReqUserLoginField reqUserLogin;

    strcpy(reqUserLogin.TradingDay, "");         // 交易日
    strcpy(reqUserLogin.BrokerID, g_chBrokerID); // 经纪公司代码
    strcpy(reqUserLogin.UserID, g_chUserID);     // 用户代码
    strcpy(reqUserLogin.Password, g_chPassword); // 密码
    strcpy(reqUserLogin.UserProductInfo, "Personal_Trader_v1"); // 用户端产品信息
    strcpy(reqUserLogin.InterfaceProductInfo, "0");  // 接口端产品信息(只须占位，不必有效赋值)
    strcpy(reqUserLogin.ProtocolInfo, "0");          // 协议信息(只须占位，不必有效赋值)

    // 发出登陆请求
    int ret = 0;
    if ((ret = m_pUserApi->ReqUserLogin(&reqUserLogin, m_nRequestID++)) != 0);
    {
        LOGERR << LOGBEG
            << "ReqUserLogin fail, ret=" << ret
            << " (" << GetCtpReqResultCodeMsg(ret) << ")"
            << LOGEND;
        return;
    }
    LOG << LOGBEG << "end." << LOGEND;
}

// 当客户端与交易托管系统通信连接断开时，该方法被调用
void CJeactpTrader::OnFrontDisconnected(int nReason)
{
    // 当发生这个情况后，API会自动重新连接，客户端可不做处理
    LOGERR << LOGBEG << "begin..." << LOGEND;
    LOGERR << LOGBEG << "end." << LOGEND;
}

// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
void CJeactpTrader::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG << LOGBEG << "begin..." << LOGEND;
    LOG << LOGBEG 
        << "ErrorCode=" << pRspInfo->ErrorID
        << ", ErrorMsg=" << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg)
        << ", RequestID=" << nRequestID
        << ", bIsLast=" << bIsLast 
        << LOGEND;

    if (pRspInfo->ErrorID != 0) {
        // 端登失败，客户端需进行错误处理
        LOGERR << LOGBEG 
            << "ErrorCode=" << pRspInfo->ErrorID
            << ", ErrorMsg=" << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg)
            << ", RequestID=" << nRequestID
            << ", bIsLast=" << bIsLast 
            << LOGEND;
        exit(-1);
    }

    m_iFrontID = pRspUserLogin->FrontID;
    m_iSessionID = pRspUserLogin->SessionID;
    memcpy(&m_chMaxOrderRef[0], &pRspUserLogin->MaxOrderRef[0], sizeof(m_chMaxOrderRef));  // 上次登入使用过的最大OrderRef
    LOG << LOGBEG << "!! MaxOrderRef: " << pRspUserLogin->MaxOrderRef << LOGEND;
    LOG << LOGBEG << "!! m_chMaxOrderRef: " << m_chMaxOrderRef << LOGEND;

    LOG << LOGBEG << "Login success!" << LOGEND;
    m_oTraderState.SetLoginTime();

    // 进行结算信息确认
    My_ReqQrySettlementInfoConfirm();

    LOG << LOGBEG << "end." << LOGEND;
}

// 报单录入应答
void CJeactpTrader::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG << LOGBEG << "begin..." << LOGEND;
    // 输出报单录入结果
    if (0 != pRspInfo->ErrorID)
    {
        LOGERR << LOGBEG << "ErrorCode=" << pRspInfo->ErrorID 
            << ", ErrorMsg=" << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg) 
            << LOGEND;
        return;
    }

    LOG << LOGBEG << "ErrorCode=" << pRspInfo->ErrorID 
        << ", ErrorMsg=" << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg) 
        << LOGEND;
    // 通知报单录入完成
    //SetEvent(g_hEvent);
    LOG << LOGBEG << "end." << LOGEND;
}

///报单回报(交易所回报, 委托回报)
void CJeactpTrader::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    LOG << LOGBEG << "begin..." << LOGEND;

    SOrderInfo* pOrderInfo = GetHistoryOrder(pOrder->OrderRef);
    if (NULL != pOrderInfo)
    {
        if (0 == pOrderInfo->m_uiTickCntFirstRtnOrder)
        {
            pOrderInfo->m_uiTickCntFirstRtnOrder = jealib::GetTickCount();
        } 
    }
    else
    {
        LOGERR << LOGBEG << "OrderRef '" << pOrder->OrderRef << "' NOT in HistoryOrder map!" << LOGEND;
    }

    LOG << LOGBEG << "OrderSysID=" << pOrder->OrderSysID << LOGEND;

    LOG << LOGBEG 
        << "\n经纪公司代码       BrokerID             :" << pOrder->BrokerID
        << "\n投资者代码         InvestorID           : " << pOrder->InvestorID 
        << "\n合约代码           InstrumentID         : " << pOrder->InstrumentID
        << "\n报单引用           OrderRef             : " << pOrder->OrderRef
        << "\n用户代码           UserID               : " << pOrder->UserID
        << "\n报单价格条件       OrderPriceType       : " << pOrder->OrderPriceType
        << "\n买卖方向           Direction            : " << pOrder->Direction
        << "\n组合开平标志       CombOffsetFlag       : " << pOrder->CombOffsetFlag
        << "\n组合投机套保标志   CombHedgeFlag        : " << pOrder->CombHedgeFlag
        << "\n价格               LimitPrice           : " << pOrder->LimitPrice
        << "\n数量               VolumeTotalOriginal  : " << pOrder->VolumeTotalOriginal
        << "\n有效期类型         TimeCondition        : " << pOrder->TimeCondition
        << "\nGTD 日期           GTDDate              : " << pOrder->GTDDate
        << "\n成交量类型         VolumeCondition      : " << pOrder->VolumeCondition
        << "\n最小成交量         MinVolume            : " << pOrder->MinVolume
        << "\n触发条件           ContingentCondition  : " << pOrder->ContingentCondition
        << "\n止损价             StopPrice            : " << pOrder->StopPrice
        << "\n强平原因           ForceCloseReason     : " << pOrder->ForceCloseReason
        << "\n自动挂起标志       IsAutoSuspend        : " << pOrder->IsAutoSuspend
        << "\n业务单元           BusinessUnit         : " << pOrder->BusinessUnit
        << "\n请求编号           RequestID            : " << pOrder->RequestID
        << "\n本地报单编号       OrderLocalID         : " << pOrder->OrderLocalID
        << "\n交易所代码         ExchangeID           : " << pOrder->ExchangeID
        << "\n会员代码           ParticipantID        : " << pOrder->ParticipantID
        << "\n客户代码           ClientID             : " << pOrder->ClientID
        << "\n合约在交易所的代码 ExchangeInstID       : " << pOrder->ExchangeInstID
        << "\n交易所交易员代码   TraderID             : " << pOrder->TraderID
        << "\n安装编号           InstallID            : " << pOrder->InstallID
        << "\n报单提交状态       OrderSubmitStatus    : " << pOrder->OrderSubmitStatus
        << "\n报单提示序号       NotifySequence       : " << pOrder->NotifySequence
        << "\n交易日             TradingDay           : " << pOrder->TradingDay
        << "\n结算编号           SettlementID         : " << pOrder->SettlementID
        << "\n报单编号           OrderSysID           : " << pOrder->OrderSysID
        << "\n报单来源           OrderSource          : " << pOrder->OrderSource
        << "\n报单状态           OrderStatus          : " << pOrder->OrderStatus
        << "\n报单类型           OrderType            : " << pOrder->OrderType
        << "\n今成交数量         VolumeTraded         : " << pOrder->VolumeTraded
        << "\n剩余数量           VolumeTotal          : " << pOrder->VolumeTotal
        << "\n报单日期           InsertDate           : " << pOrder->InsertDate
        << "\n插入时间           InsertTime           : " << pOrder->InsertTime
        << "\n激活时间           ActiveTime           : " << pOrder->ActiveTime
        << "\n挂起时间           SuspendTime          : " << pOrder->SuspendTime
        << "\n最后修改时间       UpdateTime           : " << pOrder->UpdateTime
        << "\n撤销时间           CancelTime           : " << pOrder->CancelTime
        << "\n最后修改交易所交易员代码 ActiveTraderID : " << pOrder->ActiveTraderID
        << "\n结算会员编号       ClearingPartID       : " << pOrder->ClearingPartID
        << "\n序号               SequenceNo           : " << pOrder->SequenceNo
        << "\n前置编号           FrontID              : " << pOrder->FrontID
        << "\n会话编号           SessionID            : " << pOrder->SessionID
        << "\n用户端产品信息     UserProductInfo      : " << pOrder->UserProductInfo
        << "\n状态信息           StatusMsg            : " << jealib::StringConvert::AnsiToUtf8(pOrder->StatusMsg)
        << LOGEND;

    LOG << LOGBEG << "end." << LOGEND;
}

// 成交回报
void CJeactpTrader::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    LOG << LOGBEG << "begin..." << LOGEND;

    SOrderInfo* pOrderInfo = GetHistoryOrder(pTrade->OrderRef);
    if (NULL != pOrderInfo)
    {
        pOrderInfo->m_uiTickCntRtnTrade = jealib::GetTickCount();

        LOG << LOGBEG
            << "OrderRef " << pTrade->OrderRef << " 各环节耗时统计: "
            << "行情接收->计算处理: " << jealib::GetCostTime(pOrderInfo->m_uiTickCntToWorker, pOrderInfo->m_uiTickCntRcv) 
            << "ms, 模型计算: " << jealib::GetCostTime(pOrderInfo->m_uiTickCntSendOrder, pOrderInfo->m_uiTickCntToWorker) 
            << "ms, 报单->报单确认: " << jealib::GetCostTime(pOrderInfo->m_uiTickCntFirstRtnOrder, pOrderInfo->m_uiTickCntSendOrder) 
            << "ms, 报单确认->成交: " << jealib::GetCostTime(pOrderInfo->m_uiTickCntRtnTrade, pOrderInfo->m_uiTickCntFirstRtnOrder)
            << "ms"
            << LOGEND;
    }
    else
    {
        LOGERR << LOGBEG << "OrderRef '" << pTrade->OrderRef << "' NOT in HistoryOrder map!" << LOGEND;
    }

    LOG << LOGBEG << "成交回报信息:"
        << "\n经纪公司代码       BrokerID       :" << pTrade->BrokerID
        << "\n投资者代码         InvestorID     :" << pTrade->InvestorID
        << "\n合约代码           InstrumentID   :" << pTrade->InstrumentID
        << "\n报单引用           OrderRef       :" << pTrade->OrderRef
        << "\n用户代码           UserID         :" << pTrade->UserID
        << "\n交易所代码         ExchangeID     :" << pTrade->ExchangeID
        << "\n成交编号           TradeID        :" << pTrade->TradeID
        << "\n买卖方向           Direction      :" << pTrade->Direction
        << "\n报单编号           OrderSysID     :" << pTrade->OrderSysID
        << "\n会员代码           ParticipantID  :" << pTrade->ParticipantID
        << "\n客户代码           ClientID       :" << pTrade->ClientID
        << "\n交易角色           TradingRole    :" << pTrade->TradingRole
        << "\n合约在交易所的代码 ExchangeInstID :" << pTrade->ExchangeInstID
        << "\n开平标志           OffsetFlag     :" << pTrade->OffsetFlag
        << "\n投机套保标志       HedgeFlag      :" << pTrade->HedgeFlag
        << "\n价格               Price          :" << pTrade->Price
        << "\n数量               Volume         :" << pTrade->Volume
        << "\n成交时期           TradeDate      :" << pTrade->TradeDate
        << "\n成交时间           TradeTime      :" << pTrade->TradeTime
        << "\n成交类型           TradeType      :" << pTrade->TradeType
        << "\n成交价来源         PriceSource    :" << pTrade->PriceSource
        << "\n交易所交易员代码   TraderID       :" << pTrade->TraderID
        << "\n本地报单编号       OrderLocalID   :" << pTrade->OrderLocalID
        << "\n结算会员编号       ClearingPartID :" << pTrade->ClearingPartID
        << "\n业务单元           BusinessUnit   :" << pTrade->BusinessUnit
        << "\n序号               SequenceNo     :" << pTrade->SequenceNo
        << "\n交易日             TradingDay     :" << pTrade->TradingDay
        << "\n结算编号           SettlementID   :" << pTrade->SettlementID
        << LOGEND;

    LOG << LOGBEG << "end." << LOGEND;
}

// 报单录入错误回报。由交易托管系统主动通知客户端，该方法会被调用。
void CJeactpTrader::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
    LOG << LOGBEG << "begin..." << LOGEND;
    LOGERR << LOGBEG
        << "ErrorID: " << pRspInfo->ErrorID
        << ", ErrorMsg=" << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg) 
        << LOGEND;
    LOG << LOGBEG << "end." << LOGEND;
}

// 报单操作错误回报
void CJeactpTrader::OnErrRtnOrderAction (CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
    LOG << LOGBEG << "begin..." << LOGEND;
    LOGERR << LOGBEG
        << "ErrorID: " << pRspInfo->ErrorID
        << ", ErrorMsg=" << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg) 
        << LOGEND;
    LOG << LOGBEG << "end." << LOGEND;
}

// 针对用户请求的出错通知
void CJeactpTrader::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) 
{
    LOG << LOGBEG << "begin..." << LOGEND;
    LOGERR << LOGBEG 
        << "ErrorCode=" << pRspInfo->ErrorID 
        << ", ErrorMsg=" << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg) 
        << ", RequestID=" << nRequestID
        << ", Chain=" << bIsLast
        << LOGEND;

    // 客户端需进行错误处理
    // todo.... 

    LOG << LOGBEG << "end." << LOGEND;
}

// 模型计算
MODEL_RESULT_TYPE CJeactpTrader::ModelCalculation(HISTORY_DATA_OUTPUT_TYPE &oLastHistoryData)
{
    LOG << LOGBEG << "begin..." << LOGEND;

    MODEL_RESULT_TYPE result = MODEL_RESULT_IGNORE;

    static int cnt = 0;
    cnt++;

    if (0 == cnt % 10 && 0 != cnt % 20) result = MODEL_RESULT_BUY;
    if (0 == cnt % 20) result = MODEL_RESULT_SELL;

    LOG << LOGBEG << "end." << LOGEND;
    return result;
}

// 发送报单请求
int CJeactpTrader::SendOrder(const char* pcInstrumentID, TThostFtdcDirectionType Direction, 
    TThostFtdcPriceType LimitPrice, TThostFtdcVolumeType VolumeTotalOriginal,
    TThostFtdcOffsetFlagType OffsetFlag)
{
    LOG << LOGBEG << "begin..." << LOGEND;
    
    // 端登成功,发出报单录入请求
    CThostFtdcInputOrderField ord;
    memset(&ord, 0, sizeof(ord));

    //经纪公司代码
    strcpy(ord.BrokerID, g_chBrokerID);
    //投资者代码
    strcpy(ord.InvestorID, g_chUserID);
    // 合约代码
    strcpy(ord.InstrumentID, pcInstrumentID);
    ///报单引用
    //strcpy(ord.OrderRef, "000000000001");
    IncMaxOrderRe();
    strcpy(ord.OrderRef, m_chMaxOrderRef);
    // 用户代码
    strcpy(ord.UserID, g_chUserID);
    // 报单价格条件
    ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    // 买卖方向
    //ord.Direction = THOST_FTDC_D_Buy;
    ord.Direction = Direction;
    // 组合开平标志
    memset(&ord.CombOffsetFlag[0], sizeof(ord.CombOffsetFlag), 0);
    //ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
    ord.CombOffsetFlag[0] = OffsetFlag;
    // 组合投机套保标志
    memset(&ord.CombHedgeFlag[0], sizeof(ord.CombHedgeFlag), 0);
    ord.CombHedgeFlag[0] = THOST_FTDC_HFEN_Speculation;
    // 价格
    ord.LimitPrice = LimitPrice;
    // 数量
    ord.VolumeTotalOriginal = VolumeTotalOriginal;
    // 有效期类型
    ord.TimeCondition = THOST_FTDC_TC_GFD;
    // GTD日期
    strcpy(ord.GTDDate, "");
    // 成交量类型
    ord.VolumeCondition = THOST_FTDC_VC_AV;
    // 最小成交量
    ord.MinVolume = 0;
    // 触发条件
    ord.ContingentCondition = THOST_FTDC_CC_Immediately;
    // 止损价
    ord.StopPrice = 0;
    // 强平原因
    ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    // 自动挂起标志
    ord.IsAutoSuspend = 0;

    m_pUserApi->ReqOrderInsert(&ord, m_nRequestID++);

    LOG << LOGBEG << "end." << LOGEND;
    return 0;
}

void CJeactpTrader::IncMaxOrderRe()
{
    LOG << LOGBEG << "m_chMaxOrderRef: " << m_chMaxOrderRef << LOGEND;
    
    for (unsigned int i = strlen(m_chMaxOrderRef); i < sizeof(m_chMaxOrderRef); ++i)
    {
        m_chMaxOrderRef[i] = '0';
    }
    m_chMaxOrderRef[sizeof(m_chMaxOrderRef) - 1] = 0;

/*
    if (strlen(m_chMaxOrderRef) + 1 < sizeof(m_chMaxOrderRef))
    {
        std::ostringstream oss;
        oss << std::setw(12) << std::setfill('0') << m_chMaxOrderRef;
        memcpy(&m_chMaxOrderRef[0], oss.str().c_str(), sizeof(m_chMaxOrderRef));
    }
*/

    unsigned int k = sizeof(m_chMaxOrderRef) - 2;
    for (; k >= 0; --k)
    {
        if (m_chMaxOrderRef[k] < '0') 
        {
            m_chMaxOrderRef[k] = '0';
            break;
        }
        else if (m_chMaxOrderRef[k] >= '0' && m_chMaxOrderRef[k] < '9')
        {
            m_chMaxOrderRef[k] += 1;
            break;
        }
        else
        {
            m_chMaxOrderRef[k] = '0';
            continue;
        }
    }
    LOG << LOGBEG << "m_chMaxOrderRef: " << m_chMaxOrderRef << LOGEND;
}

std::string CJeactpTrader::GetCtpReqResultCodeMsg(int code)
{
    switch (code)
    {
    case 0 : return "成功";
    case -1: return "网络连接失败";
    case -2: return "未处理请求超过许可数";
    case -3: return "每秒发送请求数超过许可数";
    default: return "未知";
    }
}

// 查询结算信息确认
// (使用ReqQrySettlementInfoConfirm可以查询当天客户结算单确认情况，无记录返回表示当天未确认结算单)
int CJeactpTrader::My_ReqQrySettlementInfoConfirm()
{
    LOG << LOGBEG << "begin...(查询结算信息确认, 是否已确认?)" << LOGEND;
    CThostFtdcQrySettlementInfoConfirmField req;
    strcpy(req.BrokerID, g_chBrokerID);
    strcpy(req.InvestorID, g_chUserID);
    
    int ret = 0;
    if ((ret = m_pUserApi->ReqQrySettlementInfoConfirm(&req, m_nRequestID++)) != 0)
    {
        LOGERR << LOGBEG
            << "ReqQrySettlementInfoConfirm fail, ret=" << ret 
            << " (" << GetCtpReqResultCodeMsg(ret) << ")"
            << LOGEND;
        return ret;
    }
    m_oTraderState.SetSendReqQrySettlementInfoConfirmTime();

    LOG << LOGBEG << "end." << LOGEND;
    return 0;
}

// 请求查询投资者结算结果
int CJeactpTrader::My_ReqQrySettlementInfo()
{
    LOG << LOGBEG << "begin...(查询投资者结算结果)" << LOGEND;
    CThostFtdcQrySettlementInfoField req;
    strcpy(req.BrokerID, g_chBrokerID);
    strcpy(req.InvestorID, g_chUserID);
    strcpy(req.TradingDay, "");  // 不填日期，表示取上一交易日结算单
   
    int ret = 0;
    if ((ret = m_pUserApi->ReqQrySettlementInfo(&req, m_nRequestID++)) != 0)
    {
        LOGERR << LOGBEG
            << "ReqQrySettlementInfo fail, ret=" << ret 
            << " (" << GetCtpReqResultCodeMsg(ret) << ")"
            << LOGEND;
        return ret;
    }
    m_oTraderState.SetSendReqQrySettlementInfoTime();
    LOG << LOGBEG << "end." << LOGEND;
    return 0;
 
}

// 投资者结算结果确认
int CJeactpTrader::My_ReqSettlementInfoConfirm()
{
    LOG << LOGBEG << "begin...(投资者结算结果确认)" << LOGEND;
    CThostFtdcSettlementInfoConfirmField req;
    strcpy(req.BrokerID, g_chBrokerID);
    strcpy(req.InvestorID, g_chUserID);
    strcpy(req.ConfirmDate, "");  // 需不需要填？？？？????!!!!
    strcpy(req.ConfirmTime, "");

    int ret = 0;
    if ((ret = m_pUserApi->ReqSettlementInfoConfirm(&req, m_nRequestID++)) != 0)
    {
        LOGERR << LOGBEG
            << "ReqSettlementInfoConfirm fail, ret=" << ret 
            << " (" << GetCtpReqResultCodeMsg(ret) << ")"
            << LOGEND;
        return ret;
    }

    LOG << LOGBEG << "end." << LOGEND;
    return 0;
}

// 请求查询投资者结算结果响应 
void CJeactpTrader::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField *pSettlementInfo, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG << LOGBEG << "begin..." << LOGEND;

    if (NULL != pSettlementInfo)
    {
        LOG << LOGBEG
            << "投资者结算结果:"
            << "\n交易日       TradingDay   : " << pSettlementInfo->TradingDay
            << "\n结算编号     SettlementID : " << pSettlementInfo->SettlementID
            << "\n经纪公司代码 BrokerID     : " << pSettlementInfo->BrokerID
            << "\n投资者代码   InvestorID   : " << pSettlementInfo->InvestorID
            << "\n序号         SequenceNo   : " << pSettlementInfo->SequenceNo
            << "\n消息正文     Content      : \n" << jealib::StringConvert::AnsiToUtf8(pSettlementInfo->Content)
            << LOGEND;
    }
    else
    {
        LOGERR << LOGBEG << "pSettlementInfo == NULL" << LOGEND;
    }

    if (NULL != pRspInfo)
    {
        LOG << LOGBEG << "响应信息:"
            << "\n错误代码  ErrorID  : " << pRspInfo->ErrorID
            << "\n错误信息  ErrorMsg : " << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg)
            << LOGEND;
    }
    else
    {
        LOGERR << LOGBEG << "pRspInfo == NULL" << LOGEND;
    }

    LOG << LOGBEG << "nRequestID: " << nRequestID
        << ", bIsLast: " << bIsLast
        << LOGEND;

    //if (NULL != pRspInfo && 0 == pRspInfo->ErrorID)
    if (bIsLast)
    {
        // 发送确认
        My_ReqSettlementInfoConfirm();
    }
    LOG << LOGBEG << "end." << LOGEND;
}

// 投资者结算结果确认应答
void CJeactpTrader::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG << LOGBEG << "begin..." << LOGEND;
    LOG << LOGBEG
        << "投资者结算结果确认信息:"
        << "\n经纪公司代码 BrokerID    : " << pSettlementInfoConfirm->BrokerID
        << "\n投资者代码   InvestorID  : " << pSettlementInfoConfirm->InvestorID
        << "\n确认日期     ConfirmDate : " << pSettlementInfoConfirm->ConfirmDate
        << "\n确认时间     ConfirmTime : " << pSettlementInfoConfirm->ConfirmTime

        << "\n\n响应信息:"
        << "\n错误代码  ErrorID  : " << pRspInfo->ErrorID
        << "\n错误信息  ErrorMsg : " << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg)

        << "\n\nnRequestID : " << nRequestID
        << "\nbIsLast : " << bIsLast
        << LOGEND;

    if (0 == pRspInfo->ErrorID)
    {
        // 确认成功 
        m_oTraderState.has_confirmed_SettlementInfo = true;
    }
    LOG << LOGBEG << "end." << LOGEND;

}

// 查询结算确认响应 
void CJeactpTrader::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    LOG << LOGBEG << "begin..." << LOGEND;

    if (NULL != pSettlementInfoConfirm)
    {
        LOG << LOGBEG
            << "投资者结算结果确认信息:"
            << "\n经纪公司代码  BrokerID    : " << pSettlementInfoConfirm->BrokerID
            << "\n投资者代码    InvestorID  : " << pSettlementInfoConfirm->InvestorID
            << "\n确认日期      ConfirmDate : " << pSettlementInfoConfirm->ConfirmDate
            << "\n确认时间      ConfirmTime : " << pSettlementInfoConfirm->ConfirmTime

            << "(已确认!)"
            << LOGEND;

            m_oTraderState.has_confirmed_SettlementInfo = true;  // 此处可判断已确认？？

    }
    else
    {
        LOGERR << LOGBEG << "pSettlementInfoConfirm == NULL" << LOGEND;
    }

    if (NULL != pRspInfo)
    {
        LOG << LOGBEG
            << "响应信息:"
            << "\n错误代码  ErrorID  : " << pRspInfo->ErrorID
            << "\n错误信息  ErrorMsg : " << jealib::StringConvert::AnsiToUtf8(pRspInfo->ErrorMsg)
            << LOGEND;
    }
    else
    {
        LOGERR << LOGBEG << "pRspInfo == NULL" << LOGEND;
    }
    
    LOG << LOGBEG 
        << "nRequestID: " << nRequestID
        << ", bIsLast: " << bIsLast
        << LOGEND;

    if (NULL != pRspInfo && 0 == pRspInfo->ErrorID)
    {
        // 确认成功 
        m_oTraderState.has_confirmed_SettlementInfo = true;
    }
    LOG << LOGBEG << "end." << LOGEND;
}


SOrderInfo* CJeactpTrader::GetHistoryOrder(std::string strOrderRef)
{

    std::map<std::string, SOrderInfo>::iterator itr = m_oHistoryOrder.find(strOrderRef);
    if (m_oHistoryOrder.end() == itr)
    {
        return NULL;
    }
    else
    {
        return &itr->second;
    }
}





