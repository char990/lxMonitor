#include <camera/Camera.h>
#include <module/ptcpp.h>

Camera::Camera(int id)
    : id(id)
{
    trigger = pOutput[id - 1];
}

Camera::~Camera()
{
}

bool Camera::TaskTakePhoto(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        PT_WAIT_UNTIL(toTakePhoto == true);
        toTakePhoto = false;
        trigger->SetPinHigh();
        tmrTakePhoto.Setms(500);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
        trigger->SetPinLow();
        tmrTakePhoto.Setms(500);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
    };
    PT_END();
}
