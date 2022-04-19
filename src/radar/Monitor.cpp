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
            if(stalker->Vdebug()>=1)
            {
                PrintDbg(DBG_PRT, "STKR[%d] takes photo", id);
            }
        }
    }
    // ----------------- iSYS400x -----------------
    if (1)//stalker->vehicleList.vlist.size() > 0)
    // only there is vehicle in stalker, in order to filter the noise for iSys400x
    {
        isys400x->TaskRadarPoll();
        st = isys400x->GetStatus();
        if (st == RadarStatus::EVENT)
        {
            isys400x->SetStatus(RadarStatus::READY);
            // check distance
            if (CheckRange())
            {
                camera->TakePhoto();
                isys400x->SaveTarget(PHOTO_TAKEN);
                if(isys400x->Vdebug()>=1)
                {
                    PrintDbg(DBG_PRT, "ISYS[%d] takes photo", id);
                }
            }
            else
            {
                isys400x->SaveTarget(nullptr);
            }
        }
    }
    else
    {
        isys400x->TaskRadarPollReset();
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
    iSys::Vehicle *v = isys400x->targetlist.minRangeVehicle;
    if (v == nullptr || !isys400x->targetlist.IsClosing())
    {
        return false;
    }
    if (tmrRange.IsExpired() || (v->range > (lastRange + uciMonitor.iSys.rangeRise * 100) && lastRange < uciMonitor.iSys.rangeLast * 100))
    {
        TaskRangeReSet();
    }
    /*
    else if (v->range >= lastRange)
    {
        // this is noise
        if (isys400x->Vdebug())
        {
            printf("\t\t\t\t\tFALSE 1: v->range=%d, lastRange=%d\n", v->range, lastRange);
        }
        return false;
    }*/
    lastRange = v->range;
    if (uciRangeIndex >= uciMonitor.distance.size() || lastRange > uciMonitor.distance[uciRangeIndex] * 100)
    {
        if (isys400x->Vdebug()>=2)
        {
            printf("\t\t\t\t\tFALSE 2: uciRangeIndex=%d, lastRange=%d\n", uciRangeIndex, lastRange);
        }
        return false;
    }
    // should take a photo
    while (uciRangeIndex < uciMonitor.distance.size() && lastRange <= uciMonitor.distance[uciRangeIndex] * 100)
    {
        uciRangeIndex++;
    };
    return true;
}
