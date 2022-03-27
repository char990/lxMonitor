#include <unistd.h>
#include <cstdio>
#include <uci/DbHelper.h>
#include <module/MyDbg.h>

void DbHelper::Init(const char * dbPath1)
{
    dbPath = dbPath1;
    uciSettings.LoadConfig();
    PrintDash('-');
}
