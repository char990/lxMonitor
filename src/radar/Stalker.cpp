#include <radar/Stalker.h>
#include <cstdio>
#include <memory>
#include <fcntl.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Radar;
using namespace Radar::Stalker;

using namespace Utils;

/**************************DBG1*************************/
void DBG1::Init(const uint8_t *dbg1)
{
    int s = sscanf((char *)dbg1, "T%d %d %c%d %c%d %c%d %d %d",
                   &number, &id, &lastDir, &lastSp, &pkDir, &pkSp, &avgDir, &avgSp, &strength, &duration);
    if (s != 10 || number > 99 || id > 9999 ||
        (DIRX != '?' && (lastDir != DIRX || pkDir != DIRX || avgDir != DIRX)) ||
        lastSp > 999 || pkSp > 999 || avgSp > 999 ||
        strength > 99 || duration > 9999)
    {
        // something wrong
        id = -1;
    }
}

std::string DBG1::ToString()
{
    uint8_t buf[33]; // replace 0x0D with '\0'
    buf[0] = 0;
    ToString(buf);
    return std::string((char *)buf);
}

int DBG1::ToString(const uint8_t *buf)
{
    return snprintf((char *)buf, 33, "T%02d %4d %c%3d %c%3d %c%3d %2d %4d ",
                    number, id, lastDir, lastSp, pkDir, pkSp, avgDir, avgSp, strength, duration);
}

/**************************LOG*************************/
void LOG::Init(uint8_t *log)
{
    char c;
    int s = sscanf((char *)log, "LOG %d %d/%d/%d %d:%d:%d %c%c%c%c L%d P%d A%d %d %d %d",
                   &id,
                   &duration, &duration, &duration, &duration, &duration, &duration,
                   &dir, &c, &c, &c, &lastSp, &pkSp, &avgSp, &strength, &classification, &duration);
    if (s != 17 || id > 9999 ||
        (DIRX != '?' && dir != DIRX) ||
        lastSp > 999 || pkSp > 999 || avgSp > 999 ||
        strength > 99 || duration > 9999)
    {
        // something wrong
        id = -1;
    }
}

std::string LOG::ToString()
{
    uint8_t buf[60];
    ToString(buf);
    return std::string((char *)buf);
}

void LOG::ToString(uint8_t *buf)
{
    struct tm rtm;
    localtime_r(&time.tv_sec, &rtm);
    sprintf((char *)buf, "LOG %d %d/%d/%d %d:%d:%d %s L%d P%d A%d %d %d %d",
            id,
            rtm.tm_year + 1900, rtm.tm_mon + 1, rtm.tm_mday, rtm.tm_hour, rtm.tm_min, rtm.tm_sec,
            dir == 'C' ? "CLOS" : "AWAY",
            lastSp, pkSp, avgSp, strength, classification, duration);
}

/**************************VIHICLE*************************/
void VehicleList::PushDgb1(uint8_t *dbg1)
{
    if (*dbg1 == '\0')
    { // no vehicle
        gettimeofday(&time, nullptr);
        VehicleFlush(time); // as there is no vhicle, use current time to flush all vehicles in list
        SaveDBG1(time, dbg1, nullptr);
    }
    else
    {
        auto d = std::shared_ptr<DBG1>(new DBG1(dbg1));
        if (d->id != -1)
        {
            hasVehicle = true;
            if (d->number == 0 || (time.tv_sec == 0 && time.tv_usec == 0))
            {
                // this is a new cycle
                // refresh time
                gettimeofday(&time, nullptr);
                VehicleFlush(time);
            }
            d->time = time;
            bool exists = false;
            for(auto v = vlist.begin(); v!=vlist.end(); v++)
            {
                if ((*v)->Id() == d->id)
                {
                    exists = true;
                    (*v)->dbg1list.push_back(d);
                }
            }
            if(exists)
            {
                SaveDBG1(time, dbg1, "");
            }
            else
            {// new vehicle
                SaveDBG1(time, dbg1, "Photo taken");
                auto v = std::shared_ptr<Vehicle>(new Vehicle);
                v->dbg1list.push_back(d);
                vlist.push_back(v);
                newVehicle = true;
            }
        }
        else
        {
            SaveDBG1(time, dbg1, "Invalid DBG1");
        }
    }
}

void VehicleList::Print()
{

}

void VehicleList::VehicleFlush(struct timeval &lasttime)
{
    auto v = vlist.begin();
    while(v!=vlist.end())
    {
        auto t = (*v)->dbg1list.back()->time;
        if (lasttime.tv_sec != 0 && Time::TimevalSubtract(&lasttime, &t) > 1000000)
        { // vehicle disappeared
            (*v)->isGone = true;
            v = vlist.erase(v);
        }
        else
        {
            v++;
        }
    }
}

extern const char *metapath;
int VehicleList::SaveDBG1(struct timeval &time, const uint8_t *dbg1, const char * str)
{
    char buf[32];
    Time::ParseTimeToLocalStr(&time, buf);
    if (memcmp(lastdate, buf, 10) != 0)
    {
        if (csvfd > 0)
        {
            close(csvfd);
        }
        memcpy(lastdate, buf, 10);
        lastdate[10] = '\0';
        char date[9];
        date[0] = lastdate[6]; // year
        date[1] = lastdate[7];
        date[2] = lastdate[8];
        date[3] = lastdate[9];
        date[4] = lastdate[3]; // month
        date[5] = lastdate[4];
        date[6] = lastdate[0]; // date
        date[7] = lastdate[1];
        date[8] = 0;
        char csv[256];
        sprintf(csv, "mkdir -p %s/%s", metapath, date);
        csvfd = system(csv);
        if (csvfd != 0)
        {
            PrintDbg(DBG_LOG, "Can NOT mkdir '%s/%s'", metapath, date);
            csvfd = -1;
        }
        else
        {
            sprintf(csv, "%s/%s/%sDBG1.csv", metapath, date, name.c_str());
            csvfd = open(csv, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
            if (csvfd < 0)
            {
                PrintDbg(DBG_LOG, "Can NOT open '%s'", csv);
            }
        }
    }
    if (csvfd > 0)
    {
        if (*dbg1 == '\0')
        { // no vehicle
            if (hasVehicle)
            {
                hasVehicle = false;
                char xbuf[128];
                int len = snprintf(xbuf, 127, "%s,There is no vehicle", buf);
                write(csvfd, xbuf, len);
            }
        }
        else
        {
            hasVehicle = true;
            char xbuf[128];
            int len = snprintf(xbuf, 127, "%s,%s%s\n", buf, dbg1, (str!=nullptr) ? str : "");
            write(csvfd, xbuf, len);
        }
    }
    return 0;
}

/**************************Radar*************************/
StalkerStat::StalkerStat(UciRadar &uciradar)
    : IRadar(uciradar), vehicleList(uciradar.name)
{
    oprSp = new OprSp(uciradar.radarPort, uciradar.radarBps, this);
    radarStatus = RadarStatus::READY;
    ssTimeout.Setms(100);
}

StalkerStat::~StalkerStat()
{
    delete oprSp;
}

int StalkerStat::RxCallback(uint8_t *data, int len)
{
    uint8_t *p = data;
    for (int i = 0; i < len; i++)
    {
        uint8_t c = *p++;
        if (c == 'T')
        { // packet start, clear buffer
            dbg1buf[0] = c;
            dbg1len = 1;
            continue;
        }
        else if (c == '\x0D')
        {
            ssTimeout.Setms(100);
            if (dbg1len == DBG1_SIZE || dbg1len == 0)
            {
                dbg1buf[dbg1len] = '\0';
                vehicleList.PushDgb1(dbg1buf);
                radarStatus = RadarStatus::EVENT;
            }
            dbg1len = 0;
        }
        else if (dbg1len > 0)
        {
            if (dbg1len < DBG1_SIZE - 1)
            {
                dbg1buf[dbg1len++] = c;
            }
            else
            {
                dbg1len = 0;
            }
        }
    }
    return 0;
}

