#include "simplelog.h"

using namespace jealib;

int main()
{
 
    const char* s = "abc";


    LOG << LOGBEG << "111" << s 
        << 123 << 0.1 
        << true
        << false
        << 'h'
        << LOGEND;

    LOGERR << LOGBEG << "there is a error" << 12.34 << LOGEND;
    return 0;

}


