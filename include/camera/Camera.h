#pragma once

#include <gpio/GpioOut.h>
#include <gpio/GpioIn.h>
#include <module/BootTimer.h>
#include <module/IPeriodicRun.h>
#include <module/Vdebug.h>

class Camera : public IPeriodicRun, public Vdebug
{
public:
    Camera(int id); // id is 1/2/3
    ~Camera();

    bool IsTakingPhoto() { return toTakePhoto; };
    void TakePhoto() { toTakePhoto = true; };
    void ConTakePhoto() { toTakePhoto = true; conTakePhoto = true;};

    virtual void PeriodicRun() override;

    GpioIn *alarm;
    bool alarm_dbg{false};

    int Id(){return id;};
private:
    int id;
    GpioOut *takephoto;
    bool toTakePhoto{false};
    bool conTakePhoto{false};
    int taskTakePhotoLine{0};
    bool TaskTakePhoto(int *_ptLine);
    BootTimer tmrTakePhoto;
};

extern Camera * cameras[3];
