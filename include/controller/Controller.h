#pragma once

#include <module/IPeriodicRun.h>
#include <radar/iSys.h>
#include <radar/Stalker.h>
#include <camera/Camera.h>
#include <gpio/GpioOut.h>
#include <module/BootTimer.h>
#include <controller/Monitor.h>

class Controller : public IPeriodicRun
{
public:
    Controller();
    ~Controller();

    virtual void PeriodicRun() override;

private:
    BootTimer tmrTrainCrossing;
    bool isTrainCrossing{false};
};


