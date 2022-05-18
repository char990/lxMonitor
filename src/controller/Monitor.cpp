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
}

Monitor::~Monitor()
{
    delete isyscsv;
}

void Monitor::Task()
{
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
            cams[ucimonitor.camRange - 1]->TakePhoto();
            if (GetVdebug() >= 1)
            {
                PrintDbg(DBG_PRT, "Monitor[%d]-STKR takes photo", id);
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
        if (isys400x->GetStatus() == RadarStatus::EVENT && isys400x->vClos.IsValid())
        {
            vfiSysClos.PushVehicle(&isys400x->vClos);
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
                lastClosRange = isys400x->vClos.range;
            }
            if (photo == 0)
            {
                isys400x->vClos.Print(buf);
                isyscsv->SaveRadarMeta(isys400x->vClos.usec, "", buf);
            }
        }
    }
    /******************* Away *******************/
    {
        photo = 0;
        auto isys400x = isys400xs[ucimonitor.isysAway - 1];
        if (isys400x->GetStatus() == RadarStatus::EVENT && isys400x->vAway.IsValid())
        {

            vfiSysAway.PushVehicle(&isys400x->vAway);
            if (vfiSysAway.isCA) // away
            {
                if (IsNewiSysAway())
                {
                    tmrVstopDly.IsClear();
                    lastAwayRange = -1;
                }
                if (GetVdebug() >= 3)
                {
                    PrintDbg(DBG_PRT, "Monitor[%d]: lastAwayRange=%d, isys400x->vAway.range=%d", id, lastAwayRange, isys400x->vAway.range);
                }
                if (lastAwayRange < ucimonitor.stopPass && isys400x->vAway.range >= ucimonitor.stopPass)
                {
                    photo = 1;
                    cams[ucimonitor.camVstop - 1]->TakePhoto();
                    tmrVstopDly.Clear();
                    isys400x->vAway.Print(buf);
                    isyscsv->SaveRadarMeta(isys400x->vClos.usec, "Photo taken: Vehicle passed Stop line", buf);
                    if (GetVdebug() >= 2)
                    {
                        PrintDbg(DBG_PRT, "Monitor[%d]: Photo taken: Vehicle passed Stop line, range=%d", id, isys400x->vAway.range);
                    }
                }
                else if (lastAwayRange < ucimonitor.stopTrigger && isys400x->vAway.range >= ucimonitor.stopTrigger)
                {
                    tmrVstopDly.Setms(ucimonitor.vstopDelay);
                    if (GetVdebug() >= 2)
                    {
                        PrintDbg(DBG_PRT, "Monitor[%d]: tmrVstopDly starts", id);
                    }
                }
                lastAwayRange = isys400x->vAway.range;
            }
            if (photo == 0)
            {
                isys400x->vAway.Print(buf);
                isyscsv->SaveRadarMeta(isys400x->vAway.usec, " ", buf);
            }
        }
        if (!tmrVstopDly.IsClear() && tmrVstopDly.IsExpired())
        {
            tmrVstopDly.Clear();
            cams[ucimonitor.camVstop - 1]->TakePhoto();
            isyscsv->SaveRadarMeta("Photo taken: Timer timeout", " ");
            if (GetVdebug() >= 2)
            {
                PrintDbg(DBG_PRT, "Monitor[%d]: Photo taken: Timer timeout", id);
            }
        }
    }
}

void Monitor::TaskiSys2()
{
    char buf[128];
    int photo;
    /******************* Clos *******************/
    {
        photo = 0;
        auto isys400x = isys400xs[ucimonitor.isysClos - 1];
        if (isys400x->GetStatus() == RadarStatus::EVENT && isys400x->vClos.IsValid())
        {
            vfiSysClos.PushVehicle(&isys400x->vClos);
            if (vfiSysClos.isCA) // closing
            {
                if (IsNewiSysClos())
                {
                    distanceIndex = 0;
                    tmrVstopDly.Clear();
                    lastClosRange = ucimonitor.distance[0];
                }
                if (lastClosRange > ucimonitor.stopPass && isys400x->vClos.range <= ucimonitor.stopPass)
                {
                    photo = 1;
                    cams[ucimonitor.camRange - 1]->TakePhoto();
                    tmrVstopDly.Clear();
                    isys400x->vClos.Print(buf);
                    isyscsv->SaveRadarMeta(isys400x->vClos.usec, "Photo taken: Vehicle passed Stop line", buf);
                    if (GetVdebug() >= 2)
                    {
                        PrintDbg(DBG_PRT, "Monitor[%d]: Photo taken: Vehicle passed Stop line, range=%d", id, isys400x->vClos.range);
                    }
                }
                else if (lastClosRange > ucimonitor.stopTrigger && isys400x->vClos.range <= ucimonitor.stopTrigger)
                {
                    tmrVstopDly.Setms(ucimonitor.vstopDelay);
                    if (GetVdebug() >= 2)
                    {
                        PrintDbg(DBG_PRT, "Monitor[%d]: tmrVstopDly starts", id);
                    }
                }
                else if (distanceIndex < ucimonitor.distance.size())
                {
                    if (PhotoByiSysClosDistance())
                    {
                        photo = 1;
                    }
                }
                lastClosRange = isys400x->vClos.range;
            }
            if (photo == 0)
            {
                isys400x->vClos.Print(buf);
                isyscsv->SaveRadarMeta(isys400x->vClos.usec, "", buf);
            }
        }
    }
    if (!tmrVstopDly.IsClear() && tmrVstopDly.IsExpired())
    {
        tmrVstopDly.Clear();
        cams[ucimonitor.camRange - 1]->TakePhoto();
        isyscsv->SaveRadarMeta("Photo taken: Timer timeout", " ");
        if (GetVdebug() >= 2)
        {
            PrintDbg(DBG_PRT, "Monitor[%d]: Photo taken: Timer timeout", id);
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
        if (GetVdebug() >= 2)
        {
            PrintDbg(DBG_PRT, "Monitor[%d]: Photo taken: distance[%d]=%d", id, distanceIndex, v.range);
        }
        distanceIndex = i;
        cams[ucimonitor.camRange - 1]->TakePhoto();
        char comment[128];
        sprintf(comment, "Photo taken: distance=%d", v.range);
        char buf[128];
        v.Print(buf);
        isyscsv->SaveRadarMeta(v.usec, comment, buf);
        return 1;
    }
    return 0;
}
