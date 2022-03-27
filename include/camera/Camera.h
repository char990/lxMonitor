#pragma once

#include <gpio/GpioOut.h>
#include <module/BootTimer.h>

class Camera
{
public:
    Camera(int id); // id is 1/2/3
    ~Camera();

    void TakePhoto() { toTakePhoto = true; };

    bool TaskTakePhoto() { return TaskTakePhoto(&taskTakePhotoLine); };

private:
    int id;
    GpioOut *trigger;
    bool toTakePhoto{false};
    int taskTakePhotoLine{0};
    bool TaskTakePhoto(int *_ptLine);
    BootTimer tmrTakePhoto;
};
