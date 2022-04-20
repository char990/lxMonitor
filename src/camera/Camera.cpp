#include <camera/Camera.h>
#include <module/ptcpp.h>
#include <module/MyDbg.h>

Camera *cameras[3];

const char XCAM[] = {'F', 'B', 'M'};

Camera::Camera(int id)
    : id(id)
{
    takephoto = pOutput[id - 1];
    alarm = pInput[id - 1];
}

Camera::~Camera()
{
}

void Camera::PeriodicRun()
{
    alarm->PeriodicRun();
    TaskTakePhoto(&taskTakePhotoLine);
}

#define TAKINGPHOTO_TIME 500
bool Camera::TaskTakePhoto(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        PT_WAIT_UNTIL(toTakePhoto == true);
        takephoto->SetPinLow();
        tmrTakePhoto.Setms(TAKINGPHOTO_TIME - 1);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
        takephoto->SetPinHigh();
        tmrTakePhoto.Setms(1000 - TAKINGPHOTO_TIME - 1);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
        if (conTakePhoto)
        {
            PrintDbg(DBG_PRT, "%cCAM-shot", XCAM[id - 1]);
            conTakePhoto = false;
        }
        toTakePhoto = false;
    };
    PT_END();
}
