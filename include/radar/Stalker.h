#pragma once

#include <string>
#include <memory>
#include <list>
#include <sys/time.h>

#include <radar/IRadar.h>
#include <fsworker/SaveCSV.h>
#include <module/ptcpp.h>
#include <uci/UciSettings.h>
#include <module/BootTimer.h>


namespace Radar
{
    namespace Stalker
    {
#define DIRX 'C' // 'A':away / 'C':close / '?':unknown/both
        class DBG1
        {
        public:
            DBG1(){};
            DBG1(const char *dbg1) { Init(dbg1); };
            timeval time;
            int number; // if number == 00, it means a new cycle
            int id{-1}; // if id == -1, it means this class is empty
            char lastDir;
            int lastSp;
            char pkDir;
            int pkSp;
            char avgDir;
            int avgSp;
            int strength;
            int duration;
            bool IsValid() { return id >= 0; };
            void Init(const char *dbg1);
            std::string ToString();
            int ToString(char *buf);
        };

        class LOG
        {
        public:
            LOG();
            LOG(const char *log) { Init(log); };
            timeval time;
            int id{-1};
            // int year, month, date, hour, minute, second; // time in LOG will be replaced with system time
            char dir;
            int lastSp;
            int pkSp;
            int avgSp;
            int strength;
            int classification;
            int duration;
            bool IsValid() { return id >= 0; };
            void Init(const char *log);
            std::string ToString();
            void ToString(char *buf);
        };

        class Vehicle
        {
        public:
            int Id() { return dbg1list.size() == 0 ? -1 : dbg1list.front()->id; };
            std::list<std::shared_ptr<DBG1>> dbg1list;
        };

        class VehicleList
        {
        public:
            VehicleList(std::string &name) : name(name), csv(name + "DBG1"){};
            std::list<std::shared_ptr<Vehicle>> vlist;
            void PushDgb1(const char *dbg1);
            bool newVehicle{false};
            bool hasVehicle{false};
        private:
            SaveCSV csv;
            int SaveDBG1(const char *dbg1, const char *comment=nullptr);
            void VehicleFlush(struct timeval &t);
            struct timeval time
            {
                0, 0
            };
            std::string &name;
            void Print();
        };

        /// \brief
        class StalkerStat : public IRadar
        {
        public:
            StalkerStat(UciRadar &uciradar);
            virtual ~StalkerStat();

            virtual int RxCallback(uint8_t *buf, int len) override;

            virtual bool TaskRadarPoll() override { return true; };

            bool NewVehicle() { return vehicleList.newVehicle; };
            void NewVehicle(bool v) { vehicleList.newVehicle = v; };

            VehicleList vehicleList;

        protected:

#define DBG1_SIZE 33
            uint8_t dbg1buf[DBG1_SIZE];
            int dbg1len;
        };

    }
}
