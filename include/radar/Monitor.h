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
    Monitor(int id, Camera *cam, Camera *camPs); // id is 1/2
    ~Monitor();

    void StalkerDebug(int v) { stalker->Vdebug(v); };
    void iSysDebug(int v) { isys400x->Vdebug(v); };

    // Add this to 100ms timer event, so PeriodicRun() will be called every 100ms
    virtual void PeriodicRun() override;

private:
    int id;
    UciMonitor &uciMonitor;
    Radar::iSys::iSys400x *isys400x;
    BootTimer tmrVstopDly;
    Radar::Stalker::StalkerStat *stalker;
    Camera *cam;    // camera for range capture
    Camera *camPs;  // camera for vehicle pass/stop
    int vspeed{0};
};

extern Monitor * monitors[2];
