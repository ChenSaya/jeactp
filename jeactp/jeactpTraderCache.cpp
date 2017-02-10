#include "jeactpTraderCache.h"
#include "simplelog.h"

CJeactpTraderCache::CJeactpTraderCache()
    :  m_iHistoryTicksCnt(10)
{
}

CJeactpTraderCache::~CJeactpTraderCache()
{
    HISTORY_DATA_CACHE_TYPE::iterator itr = m_oHistoryDataCache.begin();
    for (; m_oHistoryDataCache.end() != itr; itr++)
    {
        std::list<SMarketData*>* pList = itr->second;
        while (pList->size() > 0)
        {
            SMarketData* pMarketData = pList->front();
            pList->pop_front();
            delete pMarketData;
        }
        delete pList;
    }
}

int CJeactpTraderCache::SaveAndGetModelNeededData(const SMarketData &oMarketData, HISTORY_DATA_OUTPUT_TYPE &oLastHistoryData)
{
    CAutoMutexLock oAutoMutexLock(m_oLock);
    LOG << LOGBEG << "begin..." << LOGEND;
    std::string strInstrumentID = oMarketData.oDepthMarketData.InstrumentID;
    HISTORY_DATA_CACHE_TYPE::iterator itr = m_oHistoryDataCache.find(strInstrumentID);
    if (m_oHistoryDataCache.end() == itr)
    {
        std::list<SMarketData*>* pList = new std::list<SMarketData*>();
        m_oHistoryDataCache.insert(make_pair(strInstrumentID, pList));

        itr = m_oHistoryDataCache.find(strInstrumentID);
        if (m_oHistoryDataCache.end() == itr)
        {
            LOGERR << LOGBEG << "Create/Find strInstrumentID '" << strInstrumentID << "' from m_oHistoryDataCache fail!!!" << LOGEND;
            delete pList;
            return -1;
        }
    }

    std::list<SMarketData*>* pDataList = itr->second;
    // 下面在数据队列中删除最老的，加入最新的。后续考虑是否要对时间顺序做检查！！或许数据并不是按顺序到达的，会错位！！

    // delete the old Tick
    if (pDataList->size() >= m_iHistoryTicksCnt)
    {
        SMarketData* pOldMarketData = pDataList->front();
        pDataList->pop_front();
        delete pOldMarketData;
    }

    // save this Tick 
    SMarketData* pMarketData = new SMarketData();
    memcpy(pMarketData, &oMarketData, sizeof(SMarketData));
    pDataList->push_back(pMarketData);

    // get model needed data
    int ret = 0;
    if ((ret = GetData(oLastHistoryData)) != 0)
    {
        LOGERR << LOGBEG << "GetData fail ! ret=" << ret << LOGEND;
        return ret;
    }

    LOG << LOGBEG << "end." << LOGEND;
    return 0;
}

int CJeactpTraderCache::GetModelNeededData(HISTORY_DATA_OUTPUT_TYPE &oLastHistoryData) 
{
    CAutoMutexLock oAutoMutexLock(m_oLock);
    LOG << LOGBEG << "begin..." << LOGEND;
    int ret = 0;
    if ((ret = GetData(oLastHistoryData)) != 0)
    {
        LOGERR << LOGBEG << "GetData fail ! ret=" << ret << LOGEND;
        return ret;
    }

    LOG << LOGBEG << "end." << LOGEND;
    return 0;
}

int CJeactpTraderCache::GetData(HISTORY_DATA_OUTPUT_TYPE &oLastHistoryData) const
{
    LOG << LOGBEG << "begin..." << LOGEND;
    HISTORY_DATA_CACHE_TYPE::const_iterator itr = m_oHistoryDataCache.begin();
    for (; m_oHistoryDataCache.end() != itr; itr++)
    {
        std::string strInstrumentID = itr->first;
        std::list<SMarketData*>* pList = itr->second;


        // Get Output Vector of this instrument
        HISTORY_DATA_OUTPUT_TYPE::iterator out_map_itr = oLastHistoryData.find(strInstrumentID);
        if (oLastHistoryData.end() == out_map_itr)
        {
            std::vector<SMarketData> oVector; 
            oLastHistoryData.insert(make_pair(strInstrumentID, oVector));

            out_map_itr = oLastHistoryData.find(strInstrumentID);
            if (oLastHistoryData.end() == out_map_itr)
            {
                LOGERR << LOGBEG << "Insert <strInstrumentID, oVector> to oLastHistoryData fail!" << LOGEND;
                return -1;
            }
        }
        std::vector<SMarketData> &outputVector = out_map_itr->second;
        outputVector.resize(pList->size());

        // Copy data to Output Vector
        std::list<SMarketData*>::const_iterator itr2 = pList->begin();
        for (int i = 0; pList->end() != itr2; itr2++, ++i)
        {
            SMarketData* pMarketData = *itr2; 
            memcpy(&outputVector[i], pMarketData, sizeof(SMarketData));
        }
        
    }

    LOG << LOGBEG << "end." << LOGEND;
    return 0;
}

