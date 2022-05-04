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
    tmrVstopDly.Clear();
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
        int vs;
        int r = isys400x->CheckRange(vs);
        if (r == -1)
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
            if (r == uciMonitor.distance.size())
            {
                isys400x->SaveTarget(nullptr);
                isys400x->SaveMeta(PHOTO_TAKEN, "Based on speculation");
            }
            else
            {
                isys400x->SaveTarget(PHOTO_TAKEN);
            }
            if (r >= uciMonitor.distance.size() - 1)
            {
                tmrVstopDly.Setms(uciMonitor.vstopDelay);
                vspeed = vs;
                if (isys400x->Vdebug() >= 1)
                {
                    PrintDbg(DBG_PRT, "[%d] tmrVstopDly reload, vspeed=%d", id, vspeed);
                }
            }
        }
    }
    if (camera->alarm->IsFalling())
    {
        tmrVstopDly.Clear();
        camera->alarm->ClearEdge();
        vspeed = 0;
        if (isys400x->Vdebug() >= 1)
        {
            PrintDbg(DBG_PRT, "cam[%d]:Leaving", camera->Id());
        }
    }
    else
    {
        if (!tmrVstopDly.IsClear() && tmrVstopDly.IsExpired())
        {
            if (vspeed > 0 && vspeed <= uciMonitor.vstopSpeed)
            {
                // vehicle stops
                isys400x->SaveMeta(PHOTO_TAKEN, "Vehicle stops");
                if (isys400x->Vdebug() >= 1)
                {
                    PrintDbg(DBG_PRT, "cam[%d]:Vehicle stops", camera->Id());
                }
            }
            tmrVstopDly.Clear();
            vspeed = 0;
        }
    }

    // ----------------- own camera -----------------
    // cameraM
    if (cameraM != nullptr)
    {
        if (cameraM->alarm->IsFalling())
        {
            camera->alarm->ClearEdge();
        }
    }
}
