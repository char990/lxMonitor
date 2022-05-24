#include <controller/Monitor.h>

#include <uci/DbHelper.h>
#include <module/ptcpp.h>
#include <module/MyDbg.h>

using namespace Radar;
Monitor *monitors[2];

#define TMR_RANGE 3000
#define TMR_SPECULATION 1000
#if TMR_RANGE <= TMR_SPECULATION
#error TMR_RANGE should greater than TMR_SPECULATION
#endif

#define ISYS_VEHICLE_VALID_TIMEOUT (ucimonitor.vstopDelay + 5000)

bool Monitor::isTrainCrossing = false;

Monitor::Monitor(int id, Camera **cams, Radar::Stalker::StalkerTSS2 **tss2, Radar::iSys::iSys400x **isys)
    : id(id), cams(cams), isys400xs(isys), stalkers(tss2),
      ucimonitor(DbHelper::Instance().GetUciSettings().uciMonitor[id - 1])
{
    vfiSysClos.cmErr = DbHelper::Instance().GetUciSettings().uciiSys[ucimonitor.isysClos - 1].cmErr;
    if (ucimonitor.isysAway != 0)
    {
        vfiSysAway.cmErr = DbHelper::Instance().GetUciSettings().uciiSys[ucimonitor.isysAway - 1].cmErr;
    }
    char buf[32];
    sprintf(buf, "monitor%diSys", id);
    isyscsv = new SaveCSV(std::string(buf));
    camRange = cams[ucimonitor.camRange - 1];
    camVstop = cams[ucimonitor.camVstop - 1];
}

Monitor::~Monitor()
{
    delete isyscsv;
}

void Monitor::Task()
{
    if (isTrainCrossingRecord != isTrainCrossing)
    {
        if (isTrainCrossing)
        {
            isyscsv->SaveRadarMeta("Train is crossing", " ");
            PrintDbg(DBG_PRT, "Train is crossing");
        }
        else
        {
            isyscsv->SaveRadarMeta("Train left", " ");
            PrintDbg(DBG_PRT, "Train left");
        }
        isTrainCrossingRecord = isTrainCrossing;
    }

    TaskStkrClos();
    TaskStkrAway();
    if (id == 1)
    {
        TaskiSys1();
    }
    else
    {
        TaskiSys2();
    }
}

void Monitor::TaskStkrClos()
{
    auto stalker = stalkers[ucimonitor.stkrClos - 1];
    if (stalker->GetStatus() == RadarStatus::EVENT)
    {
        // when there is a new vehicle in sight, take photo
        if (stalker->NewVehicle() && ucimonitor.stkrCapture == 1)
        {
            stalker->NewVehicle(false);
            if (0) // camRange->IsTakingPhoto())
            {
                if (GetVdebug() >= 1)
                {
                    PrintDbg(DBG_PRT, "Monitor[%d]-STKR Cam busy", id);
                }
            }
            else
            {
                camRange->TakePhoto();
                if (GetVdebug() >= 1)
                {
                    PrintDbg(DBG_PRT, "Monitor[%d]-STKR takes photo", id);
                }
            }
        }
    }
}

void Monitor::TaskStkrAway()
{
}

int Monitor::CheckiSysClos(Radar::iSys::iSys400x *isys, int &speed)
{
    return 0;
#if 0
#define NO_PHOTO -1
    auto &uciradar = DbHelper::Instance().GetUciSettings().uciiSys[ucimonitor.isysClos - 1];
    int speculation = 0;
    auto Speculate_v1st = [&]
    {
        if (v1st.IsValid() && v1st.isCA && v1st.vk.speed > 5 &&
            (v1st.uciRangeIndex < distanceClos.size()) && !tmrSpeculation.IsExpired())
        { // refresh v1st based on speculation
            if (tmrSpeculation.IsClear())
            {
                tmrSpeculation.Setms(TMR_SPECULATION);
            }
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            int64_t usec = Timeval2us(tv);
            int us = usec - v1st.vk.usec;
            if (us > 0)
            {
                v1st.vk.usec = usec;
                v1st.vk.range -= RangeCM_sp_us(v1st.vk.speed, us);
                if (GetVdebug() >= 3)
                {
                    char buf[128];
                    v1st.vk.Print(buf);
                    PrintDbg(DBG_PRT, "\tv1st-Speculation:%s", buf);
                }
                speculation = 1;
            }
        }
    };

    auto *v = &isys->vClos;
    if (!v->IsValid())
    {
        if (tmrRange.IsExpired())
        {
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "tmrRange.IsExpired");
            }
            TaskRangeReset();
            return NO_PHOTO;
        }
        if (!v1st.IsValid() || v1st.vk.range < distanceClos.back())
        {
            return NO_PHOTO;
        }
        Speculate_v1st();
    }
    else
    {
        tmrRange.Setms(TMR_RANGE);
        tmrSpeculation.Clear();
        if (!v1st.IsValid())
        {
            if (v->range < distanceClos.back())
            {
                return NO_PHOTO;
            }
            v1st.Reset();
            v1st.Loadvk(v);
            speculation = 0;
        }
        else
        {
            if (v1st.uciRangeIndex == 0 && v->range < distanceClos.back())
            {
                return NO_PHOTO;
            }
            if (v2nd.IsValid())
            { // with v2nd exists, isys lost v1st
                Speculate_v1st();
                v2nd.Loadvk(v);
            }
            else
            { // no v2nd, there is only v1st
                if (v1st.vk.range < uciradar.rangeLast && v->range > v1st.vk.range + uciradar.rangeRise)
                { // range suddenly changed, means new vehicle
                    // this is v2nd
                    Speculate_v1st();
                    v2nd.Reset();
                    v2nd.Loadvk(v);
                    if (GetVdebug() >= 2)
                    {
                        PrintDbg(DBG_PRT, "\tnew v2 : v->range=%d", v->range);
                    }
                }
                else
                {
                    v1st.Loadvk(v);
                    speculation = 0;
                }
            }
        }
        if (vfiSysClos.isCA)
        {
            if (v1st.IsValid())
            {
                v1st.isCA = true;
            }
            if (v2nd.IsValid())
            {
                v2nd.isCA = true;
            }
        }
    }

    int photo = NO_PHOTO;
    if (v1st.isCA && v1st.uciRangeIndex < distanceClos.size())
    {
        if ((v1st.uciRangeIndex != 0 || v1st.vk.range > distanceClos.back()) && v1st.vk.range <= distanceClos[v1st.uciRangeIndex])
        {
            photo = 1;
            speed = v1st.vk.speed;
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "\tphoto = [1]:uciRangeIndex=[%d],range=%d", v1st.uciRangeIndex, v1st.vk.range);
            }
            while (v1st.uciRangeIndex < distanceClos.size() && v1st.vk.range <= distanceClos[v1st.uciRangeIndex])
            {
                v1st.uciRangeIndex++;
            };
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "\tv[1]st.uciRangeIndex=[%d]", v1st.uciRangeIndex);
            }
        }
    }
    if (v2nd.IsValid())
    {
        if (v2nd.isCA && v2nd.uciRangeIndex < distanceClos.size())
        {
            if (v2nd.vk.range <= distanceClos[v2nd.uciRangeIndex])
            {
                photo = 2;
                speed = v2nd.vk.speed;
                if (GetVdebug() >= 2)
                {
                    PrintDbg(DBG_PRT, "\tphoto = [2]:uciRangeIndex=[%d],range=%d", v2nd.uciRangeIndex, v2nd.vk.range);
                }
                while (v2nd.uciRangeIndex < distanceClos.size() && v2nd.vk.range <= distanceClos[v2nd.uciRangeIndex])
                {
                    v2nd.uciRangeIndex++;
                };
            }
        }
    }

    if (photo == 1)
    {
        photo = speculation ? distanceClos.size() : v1st.uciRangeIndex - 1;
        if (speculation || v1st.uciRangeIndex == distanceClos.size()) // only v1st && speculation
        {
            TaskRangeReset();
        }
        if (v2nd.IsValid())
        {
            v1st.Clone(v2nd);
            v2nd.Reset();
            tmrSpeculation.Clear();
        }
    }
    else if (photo == 2)
    {
        v1st.Clone(v2nd);
        v2nd.Reset();
        tmrSpeculation.Clear();
        photo = v2nd.uciRangeIndex - 1;
    }
    return photo;
#endif
}

void Monitor::TaskiSys1()
{
    char buf[128];
    int photo;
    /******************* Clos *******************/
    {
        photo = 0;
        auto isys400x = isys400xs[ucimonitor.isysClos - 1];
        if (isys400x->GetStatus() == RadarStatus::EVENT && vfiSysClos.nullcnt <= VF_SIZE)
        {
            if (isys400x->vClos.IsValid())
            {
                tmriSysClosTimeout.Setms(ISYS_VEHICLE_VALID_TIMEOUT);
            }
            vfiSysClos.PushVehicle(&isys400x->vClos);
            auto &vClos = vfiSysClos.items[VF_SIZE];
            if (vfiSysClos.isCA) // closing
            {
                if (IsNewiSysClos())
                {
                    distanceIndex = 0;
                }
                if (distanceIndex < ucimonitor.distance.size())
                {
                    if (PhotoByiSysClosDistance())
                    {
                        photo = 1;
                    }
                }
                lastClosRange = vClos.range;
            }
            if (photo == 0)
            {
                vClos.Print(buf);
                isyscsv->SaveRadarMeta(vClos.usec, "", buf);
            }
        }
        else if (tmriSysClosTimeout.IsExpired())
        {
            tmriSysClosTimeout.Clear();
            vfiSysClos.Reset();
            distanceIndex = 0;
            lastClosRange = -1;
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "Monitor[%d]: iSysClos timeout", id);
            }
        }
    }
    /******************* Away *******************/
    {
        photo = 0;
        auto isys400x = isys400xs[ucimonitor.isysAway - 1];
        if (isys400x->GetStatus() == RadarStatus::EVENT && vfiSysAway.nullcnt <= VF_SIZE)
        {
            if (isys400x->vAway.IsValid())
            {
                tmriSysAwayTimeout.Setms(ISYS_VEHICLE_VALID_TIMEOUT);
            }
            vfiSysAway.PushVehicle(&isys400x->vAway);
            auto &vAway = vfiSysAway.items[VF_SIZE];
            if (vfiSysAway.isCA) // away
            {
                if (IsNewiSysAway())
                {
                    lastAwayRange = vfiSysAway.items[0].range;
                }
                if (GetVdebug() >= 3)
                {
                    PrintDbg(DBG_PRT, "Monitor[%d]: lastAwayRange=%d, isys400x->vAway.range=%d", id, lastAwayRange, vAway.range);
                }
                if (iSysAwayWillStop == 0 && vfiSysAway.isSlowdown &&
                    vAway.range < ucimonitor.stopPass &&
                    (-vAway.speed) < ucimonitor.vstopSpeed)
                {
                    iSysAwayWillStop = 1;
                    tmrVstopDly.Setms(ucimonitor.vstopDelay);
                    if (GetVdebug() >= 2)
                    {
                        PrintDbg(DBG_PRT, "Monitor[%d]: tmrVstopDly starts - Slow down(%dKm/h)", id, vAway.speed);
                    }
                }
                if (lastAwayRange < ucimonitor.stopPass && vAway.range >= ucimonitor.stopPass)
                {
                    photo = 1;
                    tmrVstopDly.Clear();
                    iSysAwayWillStop = 0;
                    vAway.Print(buf);
                    const char *camSt;
                    if (camVstop->IsTakingPhoto())
                    {
                        camSt = "Cam busy: Vehicle passed Stop line";
                    }
                    else
                    {
                        camVstop->TakePhoto();
                        camSt = "Photo taken: Vehicle passed Stop line";
                    }
                    isyscsv->SaveRadarMeta(vAway.usec, camSt, buf);
                    if (GetVdebug() >= 2)
                    {
                        PrintDbg(DBG_PRT, "Monitor[%d]: %s, range=%d", id, camSt, vAway.range);
                    }
                }
                else if (lastAwayRange < ucimonitor.stopTrigger && vAway.range >= ucimonitor.stopTrigger)
                {
                    tmrVstopDly.Setms(ucimonitor.vstopDelay);
                    if (GetVdebug() >= 2)
                    {
                        PrintDbg(DBG_PRT, "Monitor[%d]: tmrVstopDly starts - %d >>> %d", id, lastAwayRange, vAway.range);
                    }
                }
                lastAwayRange = vAway.range;
            }
            if (photo == 0)
            {
                vAway.Print(buf);
                isyscsv->SaveRadarMeta(vAway.usec, " ", buf);
            }
        }
        else if (tmriSysAwayTimeout.IsExpired() && tmrVstopDly.IsClear())
        {
            tmriSysAwayTimeout.Clear();
            vfiSysAway.Reset();
            lastAwayRange = -1;
            iSysAwayWillStop = 0;
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "Monitor[%d]: iSysAway timeout", id);
            }
        }
        if (tmrVstopDly.IsExpired())
        {
            tmrVstopDly.Clear();
            tmriSysAwayTimeout.Clear();
            iSysAwayWillStop = 0;

            const char *camSt;
            if (camVstop->IsTakingPhoto())
            {
                camSt = "Cam busy: Vehicle Stop timeout";
            }
            else
            {
                camVstop->TakePhoto();
                camSt = "Photo taken: Vehicle Stop timeout";
            }
            isyscsv->SaveRadarMeta(camSt, " ");
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "Monitor[%d]: %s", id, camSt);
            }
        }
    }
}

void Monitor::TaskiSys2()
{
    char buf[128];
    int photo;
    /******************* Clos *******************/
    photo = 0;
    auto isys400x = isys400xs[ucimonitor.isysClos - 1];
    if (isys400x->GetStatus() == RadarStatus::EVENT && vfiSysClos.nullcnt <= VF_SIZE)
    {
        if (isys400x->vClos.IsValid())
        {
            tmriSysClosTimeout.Setms(ISYS_VEHICLE_VALID_TIMEOUT);
        }
        vfiSysClos.PushVehicle(&isys400x->vClos);
        auto &vClos = vfiSysClos.items[VF_SIZE];
        if (vfiSysClos.isCA) // closing
        {
            if (IsNewiSysClos())
            {
                distanceIndex = 0;
                lastClosRange = ucimonitor.distance[0];
            }
            if (iSysClosWillStop == 0 && vfiSysClos.isSlowdown &&
                vClos.range > ucimonitor.stopPass &&
                vClos.speed < ucimonitor.vstopSpeed)
            {
                iSysClosWillStop = 1;
                tmrVstopDly.Setms(ucimonitor.vstopDelay);
                if (GetVdebug() >= 2)
                {
                    PrintDbg(DBG_PRT, "Monitor[%d]: tmrVstopDly starts - Slow down(%dKm/h)", id, vClos.speed);
                }
            }
            if (lastClosRange > ucimonitor.stopPass && vClos.range <= ucimonitor.stopPass)
            {
                photo = 1;
                tmrVstopDly.Clear();
                iSysClosWillStop = 0;
                vClos.Print(buf);
                const char *camSt;
                if (camVstop->IsTakingPhoto())
                {
                    camSt = "Cam busy: Vehicle passed Stop line";
                }
                else
                {
                    camSt = "Photo taken: Vehicle passed Stop line";
                    camVstop->TakePhoto();
                }
                isyscsv->SaveRadarMeta(vClos.usec, camSt, buf);
                if (GetVdebug() >= 2)
                {
                    PrintDbg(DBG_PRT, "Monitor[%d]: %s, range=%d", id, camSt, vClos.range);
                }
            }
            else if (lastClosRange > ucimonitor.stopTrigger && vClos.range <= ucimonitor.stopTrigger)
            {
                tmrVstopDly.Setms(ucimonitor.vstopDelay);
                if (GetVdebug() >= 2)
                {
                    PrintDbg(DBG_PRT, "Monitor[%d]: tmrVstopDly starts - %d >>> %d", id, lastClosRange, vClos.range);
                }
            }
            else if (distanceIndex < ucimonitor.distance.size() && vClos.range > ucimonitor.stopPass)
            {
                if (PhotoByiSysClosDistance())
                {
                    photo = 1;
                }
            }
            lastClosRange = vClos.range;
        }
        if (photo == 0)
        {
            vClos.Print(buf);
            isyscsv->SaveRadarMeta(vClos.usec, "", buf);
        }
    }
    else if (tmriSysClosTimeout.IsExpired() && tmrVstopDly.IsClear())
    {
        tmriSysClosTimeout.Clear();
        vfiSysClos.Reset();
        iSysClosWillStop = 0;
        distanceIndex = 0;
        lastClosRange = -1;
        if (GetVdebug() >= 2)
        {
            PrintDbg(DBG_PRT, "Monitor[%d]: iSysClos timeout", id);
        }
    }
    if (tmrVstopDly.IsExpired())
    {
        tmrVstopDly.Clear();
        iSysClosWillStop = 0;
        tmriSysClosTimeout.Clear();
        const char *camSt;
        if (camVstop->IsTakingPhoto())
        {
            camSt = "Cam busy: Vehicle Stop timeout";
        }
        else
        {
            camVstop->TakePhoto();
            camSt = "Photo taken: Vehicle Stop timeout";
        }

        isyscsv->SaveRadarMeta(camSt, " ");
        if (GetVdebug() >= 2)
        {
            PrintDbg(DBG_PRT, "Monitor[%d]: %s", id, camSt);
        }
    }
}

int Monitor::IsNewiSysClos()
{
    int r = 1;
    auto &v = vfiSysClos.items[VF_SIZE];
    if (lastClosRange >= 0)
    {
        auto &uciisys = DbHelper::Instance().GetUciSettings().uciiSys[ucimonitor.isysClos - 1];
        if (lastClosRange < uciisys.rangeLast && v.range > lastClosRange + uciisys.rangeRise)
        { // range suddenly changed, means new vehicle
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "Monitor[%d] - isys new Clos, range=%d", id, v.range);
            }
        }
        else
        {
            r = 0;
        }
    }
    return r;
}

int Monitor::IsNewiSysAway()
{
    int r = 1;
    auto &v = vfiSysAway.items[VF_SIZE];
    if (lastAwayRange >= 0)
    {
        auto &uciisys = DbHelper::Instance().GetUciSettings().uciiSys[ucimonitor.isysAway - 1];

        if (v.range < uciisys.rangeLast && v.range < lastAwayRange - uciisys.rangeRise)
        { // range suddenly changed, means new vehicle
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "Monitor[%d] - isys new Away, range=%d", id, v.range);
            }
        }
        else
        {
            r = 0;
        }
    }
    return r;
}

int Monitor::PhotoByiSysClosDistance()
{
    // distanceIndex
    auto &v = vfiSysClos.items[VF_SIZE];
    auto &uciisys = DbHelper::Instance().GetUciSettings().uciiSys[ucimonitor.isysClos - 1];
    int i;
    for (i = 0; i < ucimonitor.distance.size(); i++)
    {
        if (v.range > ucimonitor.distance[i])
        {
            break;
        }
    }
    if (i != distanceIndex)
    {
        char comment[128];
        if (camRange->IsTakingPhoto())
        {
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "Monitor[%d]: Cam busy: distance[%d]=%d", id, distanceIndex, v.range);
            }
            sprintf(comment, "Cam busy: distance=%d", v.range);
        }
        else
        {
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "Monitor[%d]: Photo taken: distance[%d]=%d", id, distanceIndex, v.range);
            }
            camRange->TakePhoto();
            sprintf(comment, "Photo taken: distance=%d", v.range);
        }
        distanceIndex = i;
        char buf[128];
        v.Print(buf);
        isyscsv->SaveRadarMeta(v.usec, comment, buf);
        return 1;
    }
    return 0;
}
