#pragma once

#include <gpio/GpioOut.h>
#include <gpio/GpioIn.h>
#include <module/BootTimer.h>
#include <module/IPeriodicRun.h>

class Camera : public IPeriodicRun
{
public:
    Camera(int id); // id is 1/2/3
    ~Camera();

    void TakePhoto() { toTakePhoto = true; };

    virtual void PeriodicRun() override;

    GpioIn * Alarm(){ return alarm;};

private:
    int id;
    GpioOut *takephoto;
    GpioIn *alarm;
    bool toTakePhoto{false};
    
    int taskTakePhotoLine{0};
    bool TaskTakePhoto(int *_ptLine);
    BootTimer tmrTakePhoto;
};
