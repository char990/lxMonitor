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
    // ----------------- STALKER -----------------
    stalker->TaskRadar();
    if (stalker->GetStatus() == RadarStatus::NO_CONNECTION)
    {
        // stalker NO_CONNECTION
        if (stalker->radarFlag & 1)
        {
            stalker->radarFlag &= ~1;
            PrintDbg(DBG_LOG,"%s NO_CONNECTION", stalker->uciradar.name.c_str());
        }
    }

    // ----------------- iSYS400x -----------------
    isys400x->TaskRadar();
    if (isys400x->GetStatus() == RadarStatus::NO_CONNECTION)
    {
        // isys400x NO_CONNECTION
        if (isys400x->radarFlag & 1)
        {
            isys400x->radarFlag &= ~1;
            PrintDbg(DBG_LOG,"%s NO_CONNECTION", isys400x->uciradar.name.c_str());
        }
    }

    //
    TaskMonitor(&taskMonitorLine);

    // ----------------- camera -----------------
    camera->TaskTakePhoto();
}

bool Monitor::TaskMonitor(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        // ----------------- STALKER -----------------
        if (stalker->GetStatus() == RadarStatus::EVENT)
        {
            stalker->SetStatus(RadarStatus::READY);
            if ((stalker->radarFlag & 1) == 0)
            {
                stalker->radarFlag |= 1;
                PrintDbg(DBG_LOG,"%s connected", stalker->uciradar.name.c_str());
            }
            if(stalker->NewVehicle())
            {
                stalker->NewVehicle(false);
                camera->TakePhoto();
            }
        }

        // ----------------- iSYS400x -----------------
        if (isys400x->GetStatus() == RadarStatus::EVENT)
        {
            isys400x->SetStatus(RadarStatus::READY);
            if ((isys400x->radarFlag & 1) == 0)
            {
                isys400x->radarFlag |= 1;
                PrintDbg(DBG_LOG,"%s connected", isys400x->uciradar.name.c_str());
            }
            auto &v = isys400x->MinRangeVehicle();
            if(v.signal>60)
            {
                printf("isys400x:%dDB, %dKM/H, %dM, angle=%dDEG\n",
                    v.signal, v.velocity, v.range, v.angle);
                TaskDistance(&taskDistanceLine);
            }
        }
        PT_YIELD();
    };
    PT_END();
}

bool Monitor::TaskDistance(int *_ptLine)
{
    auto &v = isys400x->MinRangeVehicle();
    if(tmrDistance.IsExpired())
    {
        *_ptLine = 0;
    }
    PT_BEGIN();
    while (true)
    {
        // 150M
        tmrDistance.Setms(6000);
        // 100M
        // 50M
        // 10M
    };
    PT_END();
}

