#include "jeactpThread.h"
#include "simplelog.h"

void GetAllInstrumentList(std::vector<std::string> &oInstrumentList)
{
    oInstrumentList.push_back("cu1608");
    //oInstrumentList.push_back("cu1609");
}

int main()
{
    LOG.SetLogFileName("/tmp/jeactpTraderMain");
    LOG << LOGBEG << "Program Starting ..." << LOGEND;

    std::vector<std::string> oRecvInstrumentList;
    GetAllInstrumentList(oRecvInstrumentList);

    // get BrokerID
    char chTmp[1000];
    printf("Please enter BrokerID: ");
    scanf("%s", (char*)&chTmp);
    if (strlen(chTmp) >= sizeof(g_chBrokerID))
    {   
        printf("Error: BrokerID too long! \nExit.");
        LOGERR << LOGBEG << "BrokerID too long! Exit." << LOGEND;
        return -1;
    }
    strcpy(g_chBrokerID, chTmp);

    // get userid
    printf("Please enter UserID: ");
    scanf("%s", (char*)&chTmp);
    if (strlen(chTmp) >= sizeof(g_chUserID))
    {   
        printf("Error: UserID too long! \nExit.");
        LOGERR << LOGBEG << "UserID too long! Exit." << LOGEND;
        return -1;
    }
    strcpy(g_chUserID, chTmp);

    // get password
    printf("Please enter Password: ");
    scanf("%s", (char*)&chTmp);
    if (strlen(chTmp) >= sizeof(g_chPassword))
    {   
        printf("Error: Password too long! \nExit.");
        LOGERR << LOGBEG << "Password too long! Exit." << LOGEND;
        return -1;
    }
    strcpy(g_chPassword, chTmp);

    CJeactpThreadMgr &oThreadMgr = CJeactpThreadMgr::GetInstance();
    oThreadMgr.Init(CJeactpThreadMgr::PROCESS_AS_TRADER, oRecvInstrumentList);
    oThreadMgr.MainThreadRun();

    return 0;
}



