#include "jeactpStorage.h"
#include "simplelog.h"
#include "util.h"

CJeactpStorage::CJeactpStorage()
{
    m_fileMgrList.resize(30); //每个文件管理内有加锁，起若干个文件管理对象，避免多线程操作时过度竞争
    for (size_t i = 0; i < m_fileMgrList.size(); ++i)
    {
        m_fileMgrList[i] = new CJeactvFileMgr();
    }
}

CJeactpStorage::~CJeactpStorage()
{
    for (size_t i = 0; i < m_fileMgrList.size(); ++i)
    {
        delete m_fileMgrList[i];
    }
}

unsigned int CJeactpStorage::GetFileMgrListIndex(const std::string &strFileName)
{
    int iVal = 0;
    for (size_t i = 0; i < strFileName.size(); ++i)
    {
        iVal += (((int)strFileName[i]) * 31 + 17);
    }
    return iVal % m_fileMgrList.size();
}

//发送数据到存储处理线程(即入队处理）
int CJeactpStorage::SendToStorageThreadQueue(const SMarketData &oMarketData)
{
    std::vector<char> oBuffer(sizeof(oMarketData));
    memcpy(&oBuffer[0], &oMarketData, sizeof(oMarketData));

    m_oDataQueue.Enqueue(oBuffer);
    LOG << LOGBEG << "send finished."  << LOGEND;

    return 0;
}

int CJeactpStorage::SaveDataToFile()
{
    LOG << LOGBEG << "begin..."  << LOGEND;
    //从队列读取数据
    std::vector<char> oBuffer;
    int ret = 0;
    if ((ret = m_oDataQueue.Dequeue(oBuffer)) != 0)
    {
        LOGERR << LOGBEG << "Dequeue fail."  << LOGEND;
        return ret;
    }

    SMarketData oMarketData;
    if (oBuffer.size() != sizeof(oMarketData))
    {
        LOGERR << LOGBEG << "Buffer size error. oBuffer.size=" << (int)oBuffer.size() << " expected: " << (int)sizeof(oMarketData) << LOGEND;
        return -1;
    }
    memcpy(&oMarketData, &oBuffer[0], sizeof(oMarketData));

    // File name
    std::ostringstream ossFileName;
    std::string strInstrumentID = oMarketData.oDepthMarketData.InstrumentID;
    std::string strDate = jealib::GetDate_yyyymmdd();
    ossFileName << "data/" << strInstrumentID << "/" << strInstrumentID << "_" << strDate << ".txt";

    std::string strDataLine = FormatOutputDataLine(oMarketData);

    // save to file
    CJeactvFileMgr* pFileMgr = m_fileMgrList[GetFileMgrListIndex(ossFileName.str())];
    pFileMgr->Write(ossFileName.str(), strDataLine);

    LOG << LOGBEG << "end."  << LOGEND;
    return ret;
}

void CJeactpStorage::CheckCloseFile()
{
    std::vector<CJeactvFileMgr*>::iterator itr = m_fileMgrList.begin();
    for (; itr != m_fileMgrList.end(); ++itr)
    {
        CJeactvFileMgr* pFileMgr = *itr;
        pFileMgr->CheckCloseFile();
    }

}

inline std::string FormatOutputVal(double val)
{
    if (val > 1.0e20)
    { 
        return "";
    }
    else
    {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }
}

std::string CJeactpStorage::FormatOutputDataLine(const SMarketData &oMarketData)
{
    const CThostFtdcDepthMarketDataField* pDepthMarketData = &(oMarketData.oDepthMarketData);
    std::ostringstream oss;

    //std::string strRecvTime = oMarketData.m_strRecvTime;
    std::string strRecvTime = GetCurrentTimeWithMS();
    oss << strRecvTime << ","  
        << "," << pDepthMarketData->TradingDay         // 交易日 
	<< "," << pDepthMarketData->InstrumentID       // 合约代码
	<< "," << pDepthMarketData->ExchangeID         // 交易所代码
	<< "," << pDepthMarketData->ExchangeInstID     // 合约在交易所的代码
	<< "," << pDepthMarketData->LastPrice          // 最新价
	<< "," << pDepthMarketData->PreSettlementPrice // 上次结算价
	<< "," << pDepthMarketData->PreClosePrice      // 昨收盘
	<< "," << pDepthMarketData->PreOpenInterest    // 昨持仓量
	<< "," << pDepthMarketData->OpenPrice          // 今开盘
	<< "," << pDepthMarketData->HighestPrice       // 最高价 
	<< "," << pDepthMarketData->LowestPrice        // 最低价
	<< "," << pDepthMarketData->Volume             // 数量
	<< "," << pDepthMarketData->Turnover           // 成交金额
	<< "," << pDepthMarketData->OpenInterest       // 持仓量
        << "," << FormatOutputVal(pDepthMarketData->ClosePrice)       // 今收盘
        << "," << FormatOutputVal(pDepthMarketData->SettlementPrice)  // 本次结算价
        << "," << FormatOutputVal(pDepthMarketData->UpperLimitPrice)  // 涨停板价
	<< "," << FormatOutputVal(pDepthMarketData->LowerLimitPrice)  // 跌停板价
	<< "," << FormatOutputVal(pDepthMarketData->PreDelta )        // 昨虚实度
	<< "," << FormatOutputVal(pDepthMarketData->CurrDelta)        // 今虚实度
	<< "," << pDepthMarketData->UpdateTime         // 最后修改时间
	<< "," << pDepthMarketData->UpdateMillisec     // 最后修改毫秒
	<< "," << FormatOutputVal(pDepthMarketData->BidPrice1 )  // 申买价一
	<< "," << FormatOutputVal(pDepthMarketData->BidVolume1)  // 申买量一
	<< "," << FormatOutputVal(pDepthMarketData->AskPrice1 )  // 申卖价一
	<< "," << FormatOutputVal(pDepthMarketData->AskVolume1)  // 申卖量一
        << "," << FormatOutputVal(pDepthMarketData->BidPrice2 )  // 申买价二
        << "," << FormatOutputVal(pDepthMarketData->BidVolume2)  // 申买量二
        << "," << FormatOutputVal(pDepthMarketData->AskPrice2 )  // 申卖价二 
        << "," << FormatOutputVal(pDepthMarketData->AskVolume2)  // 申卖量二
        << "," << FormatOutputVal(pDepthMarketData->BidPrice3 )  // 申买价三
        << "," << FormatOutputVal(pDepthMarketData->BidVolume3)  // 申买量三
        << "," << FormatOutputVal(pDepthMarketData->AskPrice3 )  // 申卖价三
        << "," << FormatOutputVal(pDepthMarketData->AskVolume3)  // 申卖量三
        << "," << FormatOutputVal(pDepthMarketData->BidPrice4 )  // 申买价四
        << "," << FormatOutputVal(pDepthMarketData->BidVolume4)  // 申买量四
        << "," << FormatOutputVal(pDepthMarketData->AskPrice4 )  // 申卖价四
        << "," << FormatOutputVal(pDepthMarketData->AskVolume4)  // 申卖量四
        << "," << FormatOutputVal(pDepthMarketData->BidPrice5 )  // 申买价五
        << "," << FormatOutputVal(pDepthMarketData->BidVolume5)  // 申买量五
        << "," << FormatOutputVal(pDepthMarketData->AskPrice5 )  // 申卖价五
        << "," << FormatOutputVal(pDepthMarketData->AskVolume5)  // 申卖量五
        << "," << pDepthMarketData->AveragePrice       // 当日均价
        << "," << pDepthMarketData->ActionDay          // 业务日期
        << "\n";

    return oss.str();
}


