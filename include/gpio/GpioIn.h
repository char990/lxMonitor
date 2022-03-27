#pragma once

#include "gpio_def.h"

#include "gpio/GpioEx.h"
#include "module/Debounce.h"
#include "module/IPeriodicRun.h"
#include "module/Utils.h"

class GpioIn : public IPeriodicRun, public Debounce
{
public:
    GpioIn(int true_cnt,  int false_cnt, unsigned int pin);
    ~GpioIn(void);

    virtual void PeriodicRun() override; // read pin

private:
    GpioEx gpioex;
    unsigned int pin;
};

