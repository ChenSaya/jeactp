#ifndef _JEACTPTRADERCACHE_H_
#define _JEACTPTRADERCACHE_H_

#include <string>
#include <list>
#include <vector>
#include "jeactpStruct.h"
#include "threadlock.h"

typedef std::map<std::string, std::list<SMarketData*>*>  HISTORY_DATA_CACHE_TYPE;
typedef std::map<std::string, std::vector<SMarketData> > HISTORY_DATA_OUTPUT_TYPE;

class CJeactpTraderCache
{
public:
    CJeactpTraderCache();
    ~CJeactpTraderCache();

    int SaveAndGetModelNeededData(const SMarketData &oMarketData, HISTORY_DATA_OUTPUT_TYPE &oLastHistoryData);
    int GetModelNeededData(HISTORY_DATA_OUTPUT_TYPE &oLastHistoryData);

private:
    CJeactpTraderCache(const CJeactpTraderCache &oJeactpTraderCache); // 避免拷贝构造
    int GetData(HISTORY_DATA_OUTPUT_TYPE &oLastHistoryData) const;

private:
    HISTORY_DATA_CACHE_TYPE  m_oHistoryDataCache;  // 保存所有接收数据合约的最近历史数据
    const unsigned int       m_iHistoryTicksCnt;  // Cache中保存多少个Ticks
    CMutexLock m_oLock;
};

#endif
