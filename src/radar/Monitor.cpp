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
        }
    }
    // ----------------- iSYS400x -----------------
    isys400x->TaskRadarPoll();
    st = isys400x->GetStatus();
    if (st == RadarStatus::EVENT)
    {
        isys400x->SetStatus(RadarStatus::READY);
        if (1) // stalker->vehicleList.vlist.size() > 0)
        // only there is vehicle in stalker, in order to filter the noise for iSys400x
        {
            // check distance
            if (CheckRange())
            {
                camera->TakePhoto();
                isys400x->SaveTarget(PHOTO_TAKEN);
            }
            else
            {
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

bool Monitor::CheckRange()
{
    auto mode = isys400x->uciradar.radarMode & 1;
    iSys::Vehicle *v = isys400x->targetlist.minRangeVehicle;
    if (v == nullptr)
    {
        return false;
    }
    if (tmrRange.IsExpired() || (v->range > (lastRange + uciMonitor.iSys.rangeRise) && lastRange < uciMonitor.iSys.rangeLast))
    {
        TaskRangeReSet();
    }
    else if (v->range >= lastRange)
    {
        // this is noise
        if (isys400x->Vdebug())
        {
            printf("\t\t\t\t\tFALSE 1: v->range=%d, lastRange=%d\n", v->range, lastRange);
        }
        return false;
    }
    lastRange = v->range;
    if (uciRangeIndex >= uciMonitor.distance.size() || lastRange > uciMonitor.distance[uciRangeIndex])
    {
        if (isys400x->Vdebug())
        {
            printf("\t\t\t\t\tFALSE 2: uciRangeIndex=%d, lastRange=%d\n", uciRangeIndex, lastRange);
        }
        return false;
    }
    // should take a photo
    while (uciRangeIndex < uciMonitor.distance.size() && lastRange <= uciMonitor.distance[uciRangeIndex])
    {
        uciRangeIndex++;
    };
    return true;
}
