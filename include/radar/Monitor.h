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
    UciMonitor &uciMonitor;
    Radar::iSys::iSys400x *isys400x;
    Radar::Stalker::StalkerStat *stalker;
    Camera *camera;

    /*********************CheckRange********************/
    bool CheckRange();
    void TaskRangeReSet()
    {
        uciRangeIndex = 0;
        tmrRange.Setms(60000);
    };
    BootTimer tmrRange;
    int16_t lastRange{-1};
    int uciRangeIndex{0};
};
