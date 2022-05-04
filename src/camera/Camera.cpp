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
    if (vdebug)
    {
        if (alarm->HasEdge())
        {
            if ((alarm->IsHigh() && alarm_dbg != 1) || (alarm->IsLow() && alarm_dbg != 0))
            {
                alarm_dbg = alarm->IsHigh() ? 1 : 0;
                PrintDbg(DBG_PRT, "camera[%d]-alarm[%d]", id, alarm_dbg);
            }
        }
    }

    if (id == 3)
    {
        // TODO: for Mcam only
        if (alarm->HasEdge())
        {
            alarm->ClearEdge();
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
        takephoto->SetPinLow();
        tmrTakePhoto.Setms(TAKINGPHOTO_TIME - 1);
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
        takephoto->SetPinHigh();
        tmrTakePhoto.Setms(1000 - TAKINGPHOTO_TIME - 1);
        toTakePhoto = false;
        PT_WAIT_UNTIL(tmrTakePhoto.IsExpired());
        if (conTakePhoto)
        {
            PrintDbg(DBG_PRT, "cam[%d]-shot", id);
            conTakePhoto = false;
        }
    };
    PT_END();
}
