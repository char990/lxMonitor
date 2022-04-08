#include <controller/Controller.h>
#include <module/MyDbg.h>

Controller::Controller()
{
    pMainPwr = new GpioIn(5, 5, PIN_MAIN_FAILURE);
    pBatLow = new GpioIn(5, 5, PIN_BATTERY_LOW);
    pBatOpen = new GpioIn(5, 5, PIN_BATTERY_OPEN);
}

Controller::~Controller()
{
    delete pMainPwr;
    delete pBatLow;
    delete pBatOpen;
}

void Controller::PeriodicRun()
{
    PowerMonitor();
}

void Controller::PowerMonitor()
{
    pMainPwr->PeriodicRun();
    pBatLow->PeriodicRun();
    pBatOpen->PeriodicRun();
    if (pMainPwr->HasEdge())
    {
        pMainPwr->ClearEdge();
        PrintDbg(DBG_LOG, "Main power %s", pMainPwr->IsLow()? "FAILED" : "OK");
    }
    if (pBatOpen->HasEdge())
    {
        pBatOpen->ClearEdge();
        PrintDbg(DBG_LOG, "Battery %s", pBatOpen->IsLow()? "DISCONNECTED" : "CONNECTED");
        if (pBatOpen->IsLow()) // if there is BatOpen, don not check BatLow
        {
            return;
        }
    }
    if (pBatLow->HasEdge())
    {
        pBatLow->ClearEdge();
        PrintDbg(DBG_LOG, "Battery Voltage %s", pBatLow->IsLow()? "LOW" : "OK");
    }
}
