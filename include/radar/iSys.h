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
        public: // only use 16-bit resolution
            int64_t usec;
            uint8_t signal{0}; // db, signal(0) means this container is empty
            int16_t speed;     // km/h
            int16_t range;     // cm
            int16_t angle;     // deg
            bool IsValid() { return signal > 0; };
            int Print() { return printf("S=%3d V=%3d R=%5d A=%3d\n", signal, speed, range, angle); }
            int Print(char *buf) { return sprintf(buf, "S=%3d V=%3d R=%5d A=%3d", signal, speed, range, angle); }
            void Reset() { signal = 0; };
        };

        class VehicleFilter
        {
        public:
#define VF_SIZE 2
            VehicleFilter(int cmErr) : cmErr(cmErr)
            {
                Reset();
            };
            void PushVehicle(Vehicle *v);
            Vehicle items[VF_SIZE + 1];
            bool isClosing{false};
            bool isSlowdown{false};
            int cmErr;
            void Reset();
        };

        class TargetList
        {
        public:
            TargetList(UciRadar &uciradar) : uciradar(uciradar), csv(uciradar.name + "Target"), vfilter(uciradar.cmErr){};
            void Reset();

            int flag{0}; // return of DecodeTargetFrame
            int cnt{0};
            std::vector<Vehicle> vehicles{MAX_TARGETS};

            /// \brief  decodes target list frame received from iSYS device - Note: only 16-bit.
            /// \return -1:Error; 0:NO target; 1-MAX:number of target
            int DecodeTargetFrame(uint8_t *packet, int packetLen);

            /// \brief For unit test
            void MakeTargetMsg(uint8_t *buf, int *len);

            int SaveTarget(const char *comment);
            int SaveMeta(const char *comment, const char *details);

            int Print(char *buf);
            int Print();

            /// \brief refresh minRangeVehicle and run vehicle filter to see if this is closing vehicle
            void Refresh();
            Vehicle *minRangeVehicle;
            bool IsClosing() { return vfilter.isClosing; };
            uint64_t GetPktTime() { return pktTime.tv_sec * 1000000 + pktTime.tv_usec; };

        private:
            VehicleFilter vfilter;
            int code;
            bool hasVehicle{false};
            UciRadar &uciradar;
            SaveCSV csv;
            struct timeval pktTime
            {
                0, 0
            };
        };

        class iSys400xPower : public IPeriodicRun
        {
#define ISYS_PWR_0_T 4000
#define ISYS_PWR_1_T 1000
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

        class Range
        {
        public:
            Vehicle vk;
            bool isClosing{false};
            int uciRangeIndex{0};
            void Clone(Range *v)
            {
                if (v != this)
                {
                    memcpy(this, v, sizeof(Range));
                }
            }
            void Clone(Range &v)
            {
                Clone(&v);
            }
            void Reset()
            {
                isClosing = false;
                uciRangeIndex = 0;
                vk.Reset();
            }
            bool IsValid() { return vk.IsValid(); }
            void Loadvk(Vehicle * v)
            {
                memcpy(&vk, v, sizeof(Vehicle));
            }
        };

        class iSys400x : public IRadar
        {
        public:
            iSys400x(UciRadar &uciradar, std::vector<int> &distance);
            virtual ~iSys400x();

            virtual int RxCallback(uint8_t *buf, int len) override { return 0; }; // No RxCallback for iSYS

            bool TaskRadarPoll() override;

            void Reset();

            int SaveTarget(const char *comment);

            int SaveMeta(const char *comment, const char *details);

            TargetList targetlist;

            /// \return -1:Normal; 0~disatance.size()-1:Take photo by setting; disatance.size():Take photo by speculation
            int CheckRange(int & speed);

        protected:
            std::vector<int> &distance;

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

            /*********************TaskRadarPoll_********************/
            BootTimer tmrPwrDelay;
            int pwrDelay{0};
            BootTimer tmrTaskRadar;
            int taskRadarPoll_{0};
            bool TaskRadarPoll_(int *_ptLine);
            void TaskRadarPoll_Reset();

            /*********************TaskRange********************/
            BootTimer tmrRange, tmrSpeculation;
            Range v1st, v2nd;
            void TaskRangeReset();
        };

    }
}

extern Radar::iSys::iSys400xPower *iSys400xPwr;
