#pragma once

#include <string>

#include <radar/IRadar.h>
#include <uci/UciSettings.h>
#include <module/SerialPort.h>
#include <module/IOperator.h>
#include <module/BootTimer.h>
#include <fsworker/SaveCSV.h>
#include <module/IPeriodicRun.h>

namespace Radar
{
    namespace iSys
    {
#define MAX_TARGETS 35 // 0x23
        enum class iSYS_Status
        {
            ISYS_RX_SUCCESS = 0,
            ISYS_RX_NOT_COMPLT,
            ISYS_RX_OVERFLOW,
            ISYS_RX_FRAME_ERROR,
            ISYS_RX_SUM_ERROR,
            ISYS_RX_ADDR_ERROR,
            ISYS_RX_TIMEOUT,
            ISYS_CMD_FAILD,
        };

#define MIN_SIGNAL 50
#define MIN_SPEED 0
#define MIN_RANGE 2

        class Vehicle
        {
        public:             // only use 16-bit resolution
            uint8_t signal; // db
            int16_t speed;  // km/h
            int16_t range;  // cm
            int16_t angle;  // deg
            int Print() { return printf("S=%3d V=%3d R=%5d A=%3d\n", signal, speed, range, angle); }
            int Print(char *buf) { return sprintf(buf, "S=%3d V=%3d R=%5d A=%3d", signal, speed, range, angle); }
        };

        struct VFItem
        {
            uint64_t usec;
            int speed;
            int range;
        };

        class VehicleFilter
        {
        public:
#define VF_SIZE 2
            VehicleFilter(int cmErr) : cmErr(cmErr)
            {
                bzero(&items[0], sizeof(items));
            };
            void PushVehicle(struct timeval *time, int s, int r);
            VFItem items[VF_SIZE + 1];
            bool isColsing{false};
            int cmErr;
        };

        class TargetList
        {
        public:
            TargetList(UciRadar &uciradar) : uciradar(uciradar), csv(uciradar.name + "Target"), vfilter(uciradar.cmErr){};
            int flag{0}; // return of DecodeTargetFrame
            int cnt{0};
            std::vector<Vehicle> vehicles{MAX_TARGETS};

            /// \brief  decodes target list frame received from iSYS device - Note: only 16-bit.
            /// \return -1:Error; 0:NO target; 1-MAX:number of target
            int DecodeTargetFrame(uint8_t *packet, int packetLen);

            /// \brief For unit test
            void MakeTargetMsg(uint8_t *buf, int *len);

            int SaveTarget(const char *comment);

            int Print(char *buf);
            int Print();

            /// \brief refresh minRangeVehicle and run vehicle filter to see if this is closing vehicle
            void Refresh();
            Vehicle *minRangeVehicle;
            bool IsClosing() { return vfilter.isColsing; };

        private:
            VehicleFilter vfilter;
            int code;
            bool hasVehicle{false};
            UciRadar &uciradar;
            SaveCSV csv;
            struct timeval time
            {
                0, 0
            };
        };

        class iSys400xPower : public IPeriodicRun
        {
        public:
            enum class PwrSt
            {
                MANUAL_OFF,
                AUTOPWR_OFF,
                PWR_ON
            };
            PwrSt iSysPwr{PwrSt::PWR_ON};
            void ManualOff() { iSysPwr = PwrSt::MANUAL_OFF; };
            void AutoPwrOff() { iSysPwr = PwrSt::AUTOPWR_OFF; };
            void PwrOn() { iSysPwr = PwrSt::PWR_ON; };
            virtual void PeriodicRun() override { TaskRePower_(&_ptLine); };

        private:
            bool TaskRePower_(int *_ptLine);
            int _ptLine;
            BootTimer tmrRePwr;
        };

        class iSys400x : public IRadar
        {
        public:
            iSys400x(UciRadar &uciradar);
            virtual ~iSys400x();

            virtual int RxCallback(uint8_t *buf, int len) override { return 0; }; // No RxCallback for iSYS

            bool TaskRadarPoll() override { return TaskRadarPoll_(&taskRadar_); };
            void TaskRadarPollReset();
            int SaveTarget(const char *comment);

            TargetList targetlist;

            virtual RadarStatus GetStatus() override;

        protected:
            BootTimer tmrTaskRadar;
            int taskRadar_{0};
            bool TaskRadarPoll_(int *_ptLine);

#define MAX_PACKET_SIZE (9 + MAX_TARGETS * 7 + 1)
            uint8_t packet[MAX_PACKET_SIZE];
            int packetLen;
            void SendSd2(const uint8_t *p, int len);
            void CmdReadDeviceName();
            void CmdStartAcquisition();
            void CmdStopAcquisition();
            void CmdReadTargetList();
            bool VerifyCmdAck();
            iSYS_Status ChkRxFrame(uint8_t *packet, int packetLen);
            int DecodeDeviceName();

            /// \brief Read packet. If there are more than one packet in buf, only keep the last packet
            /// \return packet length
            int ReadPacket();

            void ClearRxBuf() { oprSp->ClearRx(); };

            virtual void ReloadTmrssTimeout() override;
        };

    }
}

extern Radar::iSys::iSys400xPower *iSys400xPwr;
