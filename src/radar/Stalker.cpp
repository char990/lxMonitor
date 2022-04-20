#include <radar/Stalker.h>
#include <cstdio>

#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Radar;
using namespace Radar::Stalker;

using namespace Utils;

#define STALKER_TIMEOUT 1000 // 1000ms

/**************************DBG1*************************/
void DBG1::Init(const char *dbg1)
{
    int s = sscanf(dbg1, "T%d %d %c%d %c%d %c%d %d %d",
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
    char buf[33]; // replace 0x0D with '\0'
    buf[0] = 0;
    ToString(buf);
    return std::string(buf);
}

int DBG1::ToString(char *buf)
{
    return snprintf(buf, 33, "T%02d %4d %c%3d %c%3d %c%3d %2d %4d ",
                    number, id, lastDir, lastSp, pkDir, pkSp, avgDir, avgSp, strength, duration);
}

/**************************LOG*************************/
void LOG::Init(const char *log)
{
    char c;
    int s = sscanf(log, "LOG %d %d/%d/%d %d:%d:%d %c%c%c%c L%d P%d A%d %d %d %d",
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
    char buf[60];
    ToString(buf);
    return std::string(buf);
}

void LOG::ToString(char *buf)
{
    struct tm rtm;
    localtime_r(&time.tv_sec, &rtm);
    sprintf(buf, "LOG %d %d/%d/%d %d:%d:%d %s L%d P%d A%d %d %d %d",
            id,
            rtm.tm_year + 1900, rtm.tm_mon + 1, rtm.tm_mday, rtm.tm_hour, rtm.tm_min, rtm.tm_sec,
            dir == 'C' ? "CLOS" : "AWAY",
            lastSp, pkSp, avgSp, strength, classification, duration);
}

/**************************VIHICLE*************************/
int VehicleList::PushDgb1(const char *dbg1)
{
    if (*dbg1 == '\0')
    { // no vehicle
        if (hasVehicle)
        {
            hasVehicle = false;
            gettimeofday(&time, nullptr);
            newVehicle = false;
            if (vdebug>=3)
            {
                vlist.clear(); // as there is no vhicle, clear all vehicles in list
                Print();
            }
        }
    }
    else
    {
        auto d = std::shared_ptr<DBG1>(new DBG1(dbg1));
        if (d->id != -1)
        {
            // refresh time
            gettimeofday(&time, nullptr);
            VehicleFlush(time);
            d->time = time;
            hasVehicle = true;
            // check if it's new vehicle
            bool vehicle_exists = false;
            if (vlist.size() > 0)
            {
                for (auto v = vlist.begin(); v != vlist.end(); v++)
                {
                    if ((*v)->Id() == d->id)
                    { // exists
                        (*v)->dbg1list.push_back(d);
                        SaveDBG1(dbg1);
                        vehicle_exists = true;
                        break;
                    }
                }
            }
            if (!vehicle_exists)
            { // Not exist in list, new vehicle
                SaveDBG1(dbg1, PHOTO_TAKEN);
                auto newv = std::shared_ptr<Vehicle>(new Vehicle);
                newv->dbg1list.push_back(d);
                vlist.push_back(newv);
                newVehicle = true;
                if (vdebug>=2)
                {
                    PrintDbg(DBG_PRT, "New Vehicle:ID=%04d", d->id);
                }
                if (vdebug>=3)
                {
                    Print();
                }
            }
            return d->id;
        }
        else
        {
            // SaveDBG1(dbg1, "Invalid DBG1");
        }
    }
    return -1;
}

void VehicleList::Print()
{
    printf("vlist(%d):\n", vlist.size());
    for (auto v = vlist.begin(); v != vlist.end(); v++)
    {
        printf("\tID=%04d\n", (*v)->Id());
    }
}

void VehicleList::VehicleFlush(struct timeval &lasttime)
{
    auto v = vlist.begin();
    while (v != vlist.end())
    {
        auto t = (*v)->dbg1list.back()->time;
        if (lasttime.tv_sec != 0 && Time::TimevalSubtract(&lasttime, &t) > STALKER_TIMEOUT * 1000)
        { // vehicle disappeared
            auto id = (*v)->dbg1list.back()->id;
            v = vlist.erase(v); // erase current and iterate next
            if (vdebug>=2)
            {
                PrintDbg(DBG_PRT, "Vehicle disappeared:ID=%04d", id);
            }
            if (vdebug>=3)
            {
                Print();
            }
        }
        else
        {
            v++;
        }
    }
}

int VehicleList::SaveDBG1(const char *dbg1, const char *comment)
{
    csv.SaveRadarMeta(time, comment, dbg1);
    return 0;
}

/**************************Radar*************************/
StalkerStat::StalkerStat(UciRadar &uciradar)
    : IRadar(uciradar), vehicleList(uciradar.name)
{
    oprSp = new OprSp(uciradar.radarPort, uciradar.radarBps, this);
    radarStatus = RadarStatus::READY;
    ReloadTmrssTimeout();
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
            ReloadTmrssTimeout();
            if (dbg1len == DBG1_SIZE - 1 || dbg1len == 0)
            {
                dbg1buf[dbg1len] = '\0';
                if(vehicleList.PushDgb1((const char *)dbg1buf)>=0)
                {
                    if (dbg1len != 0 && Vdebug()>=3)
                    {
                        printf("%s\n", dbg1buf);
                    }
                }
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

bool StalkerStat::TaskRadarPoll()
{
    return true;
}

RadarStatus StalkerStat::GetStatus()
{
    if (ssTimeout.IsExpired())
    {
        dbg1len = 0;
        dbg1buf[0] = 0;
        vehicleList.PushDgb1((const char *)dbg1buf);
        radarStatus = RadarStatus::EVENT;
    }
    return radarStatus;
}

#if 0
#include <3rdparty/catch2/EnableTest.h>
#if _ENABLE_TEST_ == 1
#include <time.h>
#include <string>
#include <unistd.h>
#include <3rdparty/catch2/catch.hpp>

TEST_CASE("Class DBG1", "[DBG1]")
{
    DBG1 dbg1;

    SECTION("legal DBG1")
    {
        dbg1.Init("T00 1027 C  5 C  5 C  5 70    1 ");
        REQUIRE(dbg1.id == 1027);
        dbg1.Init("T01 1999 C  5 C  5 C  5 70   89 ");
        REQUIRE(dbg1.id == 1999);
        dbg1.Init("T02 8888 C  5 C  5 C  5 70    1 ");
        REQUIRE(dbg1.id == 8888);
    }
    SECTION("illegal DBG1")
    {
        dbg1.Init("T00 1027 A  5 C  5 C  5 70    1 ");
        REQUIRE(dbg1.id == -1);
        dbg1.Init("T00 61027 C  5 C  5 C  5 70    1 ");
        REQUIRE(dbg1.id == -1);
        dbg1.Init("T00 1027 A  5 C  5 C  5 70 10001 ");
        REQUIRE(dbg1.id == -1);
    }
}

TEST_CASE("Class VehicleList", "[VehicleList]")
{
    auto name = std::string("TESTSTALKER1");
    VehicleList vl(name);

    SECTION("legal DBG1")
    {
        vl.PushDgb1("T00 1027 C  5 C  5 C  5 70    1 ");
        REQUIRE(vl.vlist.size() == 1);
        vl.PushDgb1("T01 1999 C  5 C  5 C  5 70   89 ");
        REQUIRE(vl.vlist.size() == 2);
        vl.PushDgb1("T02 8888 C  5 C  5 C  5 70    1 ");
        REQUIRE(vl.vlist.size() == 3);
        vl.PushDgb1("T00 1027 C  5 C  5 C  5 70    2 ");
        REQUIRE(vl.vlist.size() == 3);
        vl.PushDgb1("T01 1999 C  5 C  5 C  5 70   90 ");
        REQUIRE(vl.vlist.size() == 3);
        vl.PushDgb1("T02 8888 C  5 C  5 C  5 70    2 ");
        REQUIRE(vl.vlist.size() == 3);
        printf("No vehicle\n");
        vl.PushDgb1("\0");
        REQUIRE(vl.vlist.size() == 0);
        vl.PushDgb1("T00 1027 C  5 C  5 C  5 70    1 ");
        REQUIRE(vl.vlist.size() == 1);
        vl.PushDgb1("T01 1999 C  5 C  5 C  5 70   89 ");
        REQUIRE(vl.vlist.size() == 2);
        vl.PushDgb1("T02 8888 C  5 C  5 C  5 70    1 ");
        REQUIRE(vl.vlist.size() == 3);
        vl.PushDgb1("T00 1027 C  5 C  5 C  5 70    2 ");
        REQUIRE(vl.vlist.size() == 3);
        vl.PushDgb1("T01 1999 C  5 C  5 C  5 70   90 ");
        REQUIRE(vl.vlist.size() == 3);
        vl.PushDgb1("T02 8888 C  5 C  5 C  5 70    2 ");
        REQUIRE(vl.vlist.size() == 3);
        printf("sleep(3)\n");
        sleep(3);
        printf("New vehicle\n");
        vl.PushDgb1("T00 9027 C  5 C  5 C  5 70    2 ");
        REQUIRE(vl.vlist.size() == 1);
    }
}

#endif
#endif
