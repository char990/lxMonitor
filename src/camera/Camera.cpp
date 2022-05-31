#include <camera/Camera.h>
#include <module/ptcpp.h>
#include <module/MyDbg.h>

Camera *cameras[3];

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
    if (GetVdebug()>=1)
    {
        if (alarm->IsHigh() != alarm_dbg)
        {
            alarm_dbg = alarm->IsHigh();
            Pdebug("cam[%d]-alarm[%d]", id, alarm_dbg);
        }
    }

    TaskTakePhoto(&taskTakePhotoLine);
}

#define TAKINGPHOTO_TIME 500
bool Camera::TaskTakePhoto(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        PT_WAIT_UNTIL(toTakePhoto == true);
        if (GetVdebug()>=2)
        {
            Pdebug("cam[%d]-shooting begin", id);
        }
        takephoto->SetPinLow();
        tmrTakePhoto.Setms(TAKINGPHOTO_TIME - 1);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
        takephoto->SetPinHigh();
        tmrTakePhoto.Setms(1000 - TAKINGPHOTO_TIME - 1);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
        toTakePhoto = false;
        if (GetVdebug()>=2)
        {
            Pdebug("cam[%d]-shooting end", id);
        }
        if (conTakePhoto)
        {
            if (!(GetVdebug()>=2))
            {
                Pdebug("cam[%d]-shot", id);
            }
            conTakePhoto = false;
        }
    };
    PT_END();
}
