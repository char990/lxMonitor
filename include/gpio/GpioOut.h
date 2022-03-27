#pragma once

#include "gpio_def.h"

#include "GpioEx.h"

class GpioOut
{
public:
    GpioOut(unsigned int pin, bool init_value);
    ~GpioOut(void);
    void SetPin(bool);
    void SetPinHigh(void);
    void SetPinLow(void);
    void Toggle();
    bool GetPin(void);

private:
    GpioEx gpioex;
    bool pin_v;
};

extern GpioOut * pPinCmdPower;
inline void PinCmdPowerOn() {pPinCmdPower->SetPinHigh();};
inline void PinCmdPowerOff() {pPinCmdPower->SetPinLow();};

extern GpioOut * pPinHeartbeatLed;

extern GpioOut * pPinStatusLed;

extern GpioOut * pPinWdt;
extern GpioOut * pPinRelay;
extern GpioOut * pPinMosfet1;
extern GpioOut * pPinMosfet2;
extern GpioOut * pOutput[3];

