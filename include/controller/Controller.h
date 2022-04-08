#pragma once

#include <module/IPeriodicRun.h>
#include <gpio/GpioIn.h>

class Controller : public IPeriodicRun
{
public:
    Controller();
    ~Controller();
    virtual void PeriodicRun() override;

private:
    GpioIn * pMainPwr;
    GpioIn * pBatLow;
    GpioIn * pBatOpen;

    void PowerMonitor();
};
