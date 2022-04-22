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
    Monitor(int id, Camera *camera, Camera *cameraM); // id is 1/2
    ~Monitor();

    void StalkerDebug(int v) { stalker->Vdebug(v); };
    void iSysDebug(int v) { isys400x->Vdebug(v); };

    // Add this to 100ms timer event, so PeriodicRun() will be called every 100ms
    virtual void PeriodicRun() override;

private:
    int id;
    UciMonitor &uciMonitor;
    Radar::iSys::iSys400x *isys400x;
    Radar::Stalker::StalkerStat *stalker;
    Camera *camera;
    Camera *cameraM;

    /*********************CheckRange********************/
    /// \return 0:Normal; 1:Take photo by setting; 2:Take photo by speculation
    int CheckRange();
    void TaskRangeReSet()
    {
        uciRangeIndex = 0;
        tmrRange.Setms(60000);
    };
    BootTimer tmrRange;
    int uciRangeIndex{0};
    iSys::Vehicle lastVehicle;
};

extern Monitor * monitors[2];
