#include <radar/Monitor.h>

#include <uci/DbHelper.h>
#include <module/ptcpp.h>
#include <module/MyDbg.h>

using namespace Radar;

Monitor *monitors[2];

Monitor::Monitor(int id, Camera *camera, Camera *cameraM)
    : id(id), camera(camera), cameraM(cameraM), uciMonitor(DbHelper::Instance().GetUciSettings().uciMonitor[id - 1])
{
    stalker = new Stalker::StalkerStat(uciMonitor.stalker);
    isys400x = new iSys::iSys400x(uciMonitor.iSys, uciMonitor.distance);
}

Monitor::~Monitor()
{
    delete stalker;
    delete isys400x;
}

void Monitor::PeriodicRun()
{
    RadarStatus st;
    // ----------------- STALKER -----------------
    stalker->TaskRadarPoll();
    st = stalker->GetStatus();
    if (st == RadarStatus::EVENT)
    {
        stalker->SetStatus(RadarStatus::READY);
        // when there is a new vehicle in sight, take photo
        if (stalker->NewVehicle())
        {
            stalker->NewVehicle(false);
            camera->TakePhoto();
            if (stalker->Vdebug() >= 1)
            {
                PrintDbg(DBG_PRT, "STKR[%d] takes photo", id);
            }
        }
    }
    // ----------------- iSYS400x -----------------
    isys400x->TaskRadarPoll();
    st = isys400x->GetStatus();
    if (st == RadarStatus::EVENT)
    {
        isys400x->SetStatus(RadarStatus::READY);
        // check distance
        int r = isys400x->CheckRange();
        if (r == 0)
        {
            isys400x->SaveTarget(nullptr);
        }
        else
        {
            camera->TakePhoto();
            if (isys400x->Vdebug() >= 1)
            {
                PrintDbg(DBG_PRT, "ISYS[%d] takes photo", id);
            }
            if (r == 1)
            {
                isys400x->SaveTarget(PHOTO_TAKEN);
            }
            else if (r == 2)
            {
                isys400x->SaveMeta(PHOTO_TAKEN, "Based on speculation");
                isys400x->SaveTarget(nullptr);
            }
        }
    }

    // ----------------- own camera -----------------
    // cameraM
    if (cameraM != nullptr)
    {
        auto c3 = cameraM->Alarm();
        if (c3->IsHigh())
        {
        }
    }
}

