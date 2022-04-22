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
    isys400x = new iSys::iSys400x(uciMonitor.iSys);
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
        int r = CheckRange();
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

int Monitor::CheckRange()
{
    int r = 0;
    iSys::Vehicle *v = isys400x->targetlist.minRangeVehicle;
    if (v == nullptr)
    {
        if (isys400x->targetlist.IsClosing() && lastVehicle.speed != 0)
        {
            
        
            r = 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if (tmrRange.IsExpired())
        {
            TaskRangeReSet();
            bzero(&lastVehicle, sizeof(lastVehicle));
        }
        else if((v->range > (lastVehicle.range + uciMonitor.iSys.rangeRise * 100) && lastVehicle.range < uciMonitor.iSys.rangeLast * 100))
        {
            memcpy(&lastVehicle, v, sizeof(lastVehicle));

        }
    }
    if (uciRangeIndex >= uciMonitor.distance.size() || lastVehicle.range > uciMonitor.distance[uciRangeIndex] * 100)
    {
        if (isys400x->Vdebug() >= 2)
        {
            printf("\t\tFALSE 2: uciRangeIndex=%d, lastRange=%d\n", uciRangeIndex, lastVehicle.range);
        }
        return 0;
    }
    // should take a photo
    while (uciRangeIndex < uciMonitor.distance.size() && lastVehicle.range <= uciMonitor.distance[uciRangeIndex] * 100)
    {
        uciRangeIndex++;
    };
    return r + 1;
}
