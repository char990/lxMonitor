#include <radar/Monitor.h>

#include <uci/DbHelper.h>
#include <module/ptcpp.h>
#include <module/MyDbg.h>

using namespace Radar;

Monitor::Monitor(int id)
    : id(id)
{
    camera = new Camera(id);
    auto &uciMonitor = DbHelper::Instance().GetUciSettings().uciMonitor[id - 1];
    stalker = new Stalker::StalkerStat(uciMonitor.stalker);
    isys400x = new iSys::iSys400x(uciMonitor.iSys);
}

Monitor::~Monitor()
{
    delete stalker;
    delete isys400x;
    delete camera;
}

void Monitor::PeriodicRun()
{
    RadarStatus st;
    // ----------------- STALKER -----------------
    stalker->TaskRadarPoll();
    st = stalker->GetStatus();
    if (st == RadarStatus::NO_CONNECTION)
    {
        // stalker NO_CONNECTION
        if (stalker->IsConnected())
        {
            stalker->Connected(false);
            PrintDbg(DBG_LOG, "%s NO_CONNECTION", stalker->uciradar.name.c_str());
        }
    }
    else if (st == RadarStatus::EVENT)
    {
        stalker->SetStatus(RadarStatus::READY);
        if (!stalker->IsConnected())
        {
            stalker->Connected(true);
            PrintDbg(DBG_LOG, "%s connected", stalker->uciradar.name.c_str());
        }
        if (stalker->NewVehicle())
        {
            stalker->NewVehicle(false);
            camera->TakePhoto(); // when there is a new vehicle in sight, take photo
        }
    }

    // ----------------- iSYS400x -----------------
    isys400x->TaskRadarPoll();
    st = isys400x->GetStatus();
    if (st == RadarStatus::NO_CONNECTION)
    {
        // isys400x NO_CONNECTION
        if (isys400x->IsConnected())
        {
            isys400x->Connected(false);
            PrintDbg(DBG_LOG, "%s NO_CONNECTION", isys400x->uciradar.name.c_str());
        }
    }
    else if (st == RadarStatus::EVENT)
    {
        isys400x->SetStatus(RadarStatus::READY);
        if (!isys400x->IsConnected())
        {
            isys400x->Connected(true);
            PrintDbg(DBG_LOG, "%s connected", isys400x->uciradar.name.c_str());
        }
    }
    // check distance
    TaskDistance(&taskDistanceLine);

    // ----------------- camera -----------------
    camera->TaskTakePhoto();
}

bool Monitor::TaskDistance(int *_ptLine)
{
    auto &v = isys400x->minRangeVehicle;
    if (tmrDistance.IsExpired())
    {
        *_ptLine = 0;
    }
    PT_BEGIN();
    while (true)
    {
        if (v != nullptr)
        {
            printf("isys400x:%dDB, %dKM/H, %dM, angle=%dDEG\n",
                   v->signal, v->speed, v->range, v->angle);
        }
        // 150M
        tmrDistance.Setms(6000);
        // 100M
        // 50M
        // 10M
    };
    PT_END();
}
