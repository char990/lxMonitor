#pragma once

#include <module/IPeriodicRun.h>
#include <radar/iSys.h>
#include <radar/Stalker.h>
#include <camera/Camera.h>
#include <gpio/GpioOut.h>
#include <module/BootTimer.h>

class Monitor : public IPeriodicRun
{
public:
    Monitor(int id); // id is 1/2
    ~Monitor();

    // Add this to 100ms timer event, so PeriodicRun() will be called every 100ms
    virtual void PeriodicRun() override;

private:
    int id;
    Radar::iSys::iSys400x *isys400x;
    Radar::Stalker::StalkerStat *stalker;
    Camera *camera;

    /*********************TaskDistance********************/
    int taskDistanceLine{0};
    bool TaskDistance(int *_ptLine);
    void TaskDistanceReSet() { taskDistanceLine = 0; };
    BootTimer tmrDistance;

};
