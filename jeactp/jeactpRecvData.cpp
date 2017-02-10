#include "jeactpThread.h"
#include "simplelog.h"

void GetAllInstrumentList(std::vector<std::string> &oInstrumentList)
{
    time_t now = time(0);
    tm * pTime = localtime(&now);

    const char* preList[] =
        {
            "cu", "al", "zn", "pb", "ni", "sn", "au", "ag", "rb", "wr", "hc", "fu", "bu", "ru",
            "P", "PP", "FB", "BB", "JD", "I", "JM", "V", "L", "Y", "A", "C", "B", "M", "P", "J", "CS",
            "SM", "SF", "LR", "JR", "ZC", "FG", "MA", "CF", "OI", "RM", "RS", "TA", "SR", "WH", "RI",
            "IC", "IH", "T", "TF", "IF"
        };
    int preSize = sizeof(preList) / sizeof(preList[0]);
    for (int i = 0; i < preSize; ++i)
    {
        int year  = (pTime->tm_year + 1900) % 100;
        int month = pTime->tm_mon + 1;
        for (int j = 0; j < 24; ++j)
        {
            std::stringstream oss;
            oss << preList[i]
                << std::setw(2) << std::setfill('0') << year
                << std::setw(2) << std::setfill('0') << month;
            LOG << LOGBEG << oss.str() << LOGEND;
            oInstrumentList.push_back(oss.str());
            if (month < 12)
            {
                ++month;
            }
            else
            {
                ++year;
                month = 1;
            }
        }
    }
}

int main()
{
    LOG.SetLogFileName("/tmp/jeactpRecvData.dev04");
    LOG << LOGBEG << "Program Starting ..." << LOGEND;

    std::vector<std::string> oRecvInstrumentList;
    GetAllInstrumentList(oRecvInstrumentList);
    LOG << LOGBEG << "oRecvInstrumentList size: " << (int)oRecvInstrumentList.size() << LOGEND;

    CJeactpThreadMgr &oThreadMgr = CJeactpThreadMgr::GetInstance();
    oThreadMgr.Init(CJeactpThreadMgr::PROCESS_AS_RECV_DATA, oRecvInstrumentList);
    oThreadMgr.MainThreadRun();

    return 0;
}



