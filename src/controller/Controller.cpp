#include <controller/Controller.h>

#include <uci/DbHelper.h>
#include <module/ptcpp.h>
#include <module/MyDbg.h>

using namespace Radar;

Controller::Controller()
{

}

Controller::~Controller()
{
}

void Controller::PeriodicRun()
{
    for (int i = 0; i < 2; i++)
    {
        monitors[i]->Task();
    }

    // over
    for (int i = 0; i < 2; i++)
    {
        if (stalkerTSS2[i]->GetStatus() == RadarStatus::EVENT)
        {
            stalkerTSS2[i]->SetStatus(RadarStatus::READY);
        }
        if (isys400x[i]->GetStatus() == RadarStatus::EVENT)
        {
            isys400x[i]->SetStatus(RadarStatus::READY);
        }
    }

#if 0
    if (uciTrian.monitor == id) // TODO: check if there is a train
    {
        if (camVstop->alarm->IsFalling())
        {
            tmrVstopDly.Clear();
            camVstop->alarm->ClearEdge();
            vspeed = 0;
            camVstop->TakePhoto();
            isys400x->SaveMeta(PHOTO_TAKEN, "Vehicle passed/Train is passing...");
            if (isys400x->Vdebug() >= 1)
            {
                PrintDbg(DBG_PRT, "[%d]:Vehicle passed/Train is passing...", camVstop->Id());
            }
            tmrTrainDly.Setms(1500);
        }
        else
        {
            if (camVstop->alarm->IsRising())
            {
                tmrTrainDly.Clear();
                camVstop->alarm->ClearEdge();
                isTrain = false;
            }
            else if (tmrTrainDly.IsExpired())
            {
                tmrTrainDly.Clear();
                isTrain = true;
            }

            if (!tmrVstopDly.IsClear() && tmrVstopDly.IsExpired())
            {
                if (vspeed > 0 && vspeed <= uciMonitor.vstopSpeed)
                {
                    // vehicle stops
                    camVstop->TakePhoto();
                    isys400x->SaveMeta(PHOTO_TAKEN, "Vehicle stops");
                    if (isys400x->Vdebug() >= 1)
                    {
                        PrintDbg(DBG_PRT, "[%d]:Vehicle stops", camVstop->Id());
                    }
                }
                tmrVstopDly.Clear();
                vspeed = 0;
            }
        }
    }
    else // check if vehicle is crossing line or stops
    {
        if (camVstop->alarm->IsFalling())
        {
            camVstop->alarm->ClearEdge();
            tmrVstopDly.Clear();
            vspeed = 0;
            camVstop->TakePhoto();
            isys400x->SaveMeta(PHOTO_TAKEN, "Vehicle passed");
            if (isys400x->Vdebug() >= 1)
            {
                PrintDbg(DBG_PRT, "[%d]:Vehicle passed", camVstop->Id());
            }
        }
        else
        {
            if (!tmrVstopDly.IsClear() && tmrVstopDly.IsExpired())
            {
                if (vspeed > 0 && vspeed <= uciMonitor.vstopSpeed)
                {
                    // vehicle stops
                    camVstop->TakePhoto();
                    isys400x->SaveMeta(PHOTO_TAKEN, "Vehicle stops");
                    if (isys400x->Vdebug() >= 1)
                    {
                        PrintDbg(DBG_PRT, "[%d]:Vehicle stops", camVstop->Id());
                    }
                }
                tmrVstopDly.Clear();
                vspeed = 0;
            }
        }
    }
#endif
}
