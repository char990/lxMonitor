#include <radar/Monitor.h>

#include <uci/DbHelper.h>
#include <module/ptcpp.h>
#include <module/MyDbg.h>

using namespace Radar;

Monitor::Monitor(int id)
    : id(id), uciMonitor(DbHelper::Instance().GetUciSettings().uciMonitor[id - 1])
{
    camera = new Camera(id);
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
        // check distance
        if(CheckRange())
        {
            camera->TakePhoto();
            isys400x->SaveTarget(PHOTO_TAKEN);
        }
        else
        {
            isys400x->SaveTarget(nullptr);
        }
    }
    // ----------------- camera -----------------
    camera->PeriodicRun();
}

bool Monitor::CheckRange()
{
    auto &v = isys400x->minRangeVehicle;
    if (v == nullptr)
    {
        return false;
    }
    if (tmrRange.IsExpired() || (v->range > (lastRange + 3) && v->range <50))
    {
        TaskRangeReSet();
    }
    lastRange = v->range;
    if(lastRange > uciMonitor.distance[uciRangeIndex])
    {
        return false;
    }
    // should take a photo
    do
    {
        if ( ++ uciRangeIndex >= uciMonitor.distance.size())
        { // last
            TaskRangeReSet();
            break;
        }
    }while(lastRange < uciMonitor.distance[uciRangeIndex]);
    return true;
}
