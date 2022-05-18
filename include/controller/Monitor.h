#pragma once

#include <module/IPeriodicRun.h>
#include <radar/iSys.h>
#include <radar/Stalker.h>
#include <camera/Camera.h>
#include <gpio/GpioOut.h>
#include <module/BootTimer.h>
#include <module/Vdebug.h>
#include <fsworker/SaveCSV.h>


class Monitor : public Vdebug
{
public:
    Monitor(int id, Camera **cams, Radar::Stalker::StalkerTSS2 **tss2, Radar::iSys::iSys400x **isys);
    ~Monitor();

    void Task();

private:
    int id;
    SaveCSV *isyscsv;

    void TaskStkrClos();
    void TaskStkrAway();
    void TaskiSys2();
    void TaskiSys1();

    UciMonitor &ucimonitor;
    Radar::iSys::iSys400x **isys400xs;
    Radar::Stalker::StalkerTSS2 **stalkers;
    Camera **cams;

    /*********************iSysClos********************/
    Radar::iSys::VehicleFilter vfiSysClos;
    int lastClosRange{-1};
    int IsNewiSysClos();
    int PhotoByiSysClosDistance();
    int distanceIndex{0};

    /*********************iSysAway********************/
    Radar::iSys::VehicleFilter vfiSysAway;
    int lastAwayRange{-1};
    BootTimer tmrVstopDly;
    int IsNewiSysAway();

    /*********************TaskRange********************/
    BootTimer tmrRange, tmrSpeculation;
    Radar::iSys::Range v1st, v2nd;
    void TaskRangeReset()
    {
        tmrRange.Clear();
        tmrSpeculation.Clear();
        v1st.Reset();
        v2nd.Reset();
    };

    int CheckiSysClos(Radar::iSys::iSys400x *isys, int &vs);
};


extern Monitor *monitors[2];