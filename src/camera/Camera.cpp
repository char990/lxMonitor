#include <camera/Camera.h>
#include <module/ptcpp.h>
#include <module/MyDbg.h>


Camera * cameras[3];

const char XCAM[]={'F', 'B', 'M'};

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

bool Camera::TaskTakePhoto(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        PT_WAIT_UNTIL(toTakePhoto == true);
        toTakePhoto = false;
        takephoto->SetPinHigh();
        tmrTakePhoto.Setms(500-1);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
        PrintDbg(DBG_PRT, "%cCAM-shot", XCAM[id-1]);
        takephoto->SetPinLow();
        tmrTakePhoto.Setms(500-1);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
    };
    PT_END();
}
