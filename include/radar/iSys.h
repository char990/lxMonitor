#pragma once

#include <string>

#include <radar/IRadar.h>
#include <uci/UciSettings.h>
#include <module/SerialPort.h>
#include <module/IOperator.h>
#include <module/BootTimer.h>
#include <fsworker/SaveCSV.h>
#include <module/IPeriodicRun.h>
#include <3rdparty/Kalman/TrivialKalmanFilter.h>

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

#define RangeCM_sp_us(sp, us) (sp * us / 36000)
#define Timeval2us(tv) ((int64_t)1000000 * tv.tv_sec + tv.tv_usec)

        class Vehicle
        {
        public: // only use 16-bit resolution
            int64_t usec;
            uint8_t signal{0}; // db, signal(0) means this container is empty
            int16_t speed;     // km/h
            int16_t range;     // cm
            int16_t angle;     // deg
            bool IsValid() { return signal > 0; };
            int Print();
            int Print(char *buf);
            void Reset() { signal = 0; };
        };

        class VehicleFilter : public Vdebug
        {
        public:
#define VF_SIZE 2   // must >= 2
            VehicleFilter();
            // VehicleFilter(int cmErr);
            bool IsValid(){return nullcnt<3;};
            TrivialKalmanFilter<float> tkf_speed;
            TrivialKalmanFilter<float> tkf_range;
            int nullcnt{0};
            void PushVehicle(Vehicle *v);
            Vehicle items[VF_SIZE + 1];
            bool isCA{false}; // Closing or Away confirmed
            bool isSlowdown{false};
            bool isSpeedUp{false};
            int cmErr;
            void Reset();
        };

#if 0
        class TargetList
        {
        public:
            TargetList(UciRadar &uciradar) : uciradar(uciradar), csv(uciradar.name + "Target"), vfilter(uciradar.cmErr){};
            void Reset();

            int flag{0}; // return of DecodeTargetFrame
            int cnt{0};
            Vehicle vClos;
            Vehicle vAway;

            /// \brief  decodes target list frame received from iSYS device - Note: only 16-bit.
            /// \return -1:Error; 0:NO target; 1-MAX:number of target
            int DecodeTargetFrame(uint8_t *packet, int packetLen);

            /// \brief For unit test
            void MakeTargetMsg(uint8_t *buf, int *len);

            int SaveTarget(const char *comment);
            int SaveMeta(const char *comment, const char *details);

            int Print(char *buf);
            int Print();

            Vehicle *minRangeVehicle;
            bool IsClosing() { return vfilter.isCA; };
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
#endif
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
            bool isCA{false};
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
                isCA = false;
                uciRangeIndex = 0;
                vk.Reset();
            }
            bool IsValid() { return vk.IsValid(); }
            void Loadvk(Vehicle *v)
            {
                memcpy(&vk, v, sizeof(Vehicle));
            }
        };

        class iSys400x : public IRadar, public IPeriodicRun
        {
        public:
            iSys400x(UciRadar &uciradar);
            virtual ~iSys400x();

            virtual int RxCallback(uint8_t *buf, int len) override { return 0; }; // No RxCallback for iSYS

            virtual void PeriodicRun() override { TaskRadarPoll(); };

            bool TaskRadarPoll() override;

            void Reset();

            Vehicle vClos; // closing vehicle: speed & range > 0
            Vehicle vAway; // away vehicle: speed & range < 0

            /// \brief  decodes target list frame received from iSYS device - Note: only 16-bit.
            /// \return -1:Error; 0:NO target; 1:Closing; 2:Away; 3:Both
            int DecodeTargetFrame(/*uint8_t *packet, int packetLen*/);
#if 0
            int SaveTarget(const char *comment);

            int SaveMeta(const char *comment, const char *details);

            /// \return -1:Normal; 0~disatance.size()-1:Take photo by setting; disatance.size():Take photo by speculation
            int CheckRange(int &speed);
#endif
        protected:
            // std::vector<int> &distance;
            struct timeval pktTime
            {
                0, 0
            };
#define MAX_PACKET_SIZE (9 + MAX_TARGETS * 7 + 1)
            uint8_t packet[MAX_PACKET_SIZE];
            int packetLen;
            void SendSd2(const uint8_t *p, int len);
            void CmdReadDeviceName();
            void CmdStartAcquisition();
            void CmdStopAcquisition();
            void CmdReadFreqChannel();
            void CmdSetFreqChannel();
            void CmdReadTargetList();
            void CmdReadAppSetting(uint8_t index);
            void CmdWriteAppSetting(uint8_t index, int16_t data);
            int DecodeAppSetting();
            void CmdSaveToEEprom();

            bool VerifyCmdAck(uint8_t cmd);
            iSYS_Status ChkRxFrame(uint8_t *packet, int packetLen);
            int DecodeDeviceName();
            int DecodeFreqChannel();

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

            // Settings tmr
            BootTimer tmrTaskRadarSettings;
            bool writeToEEP{false};
            /*********************TaskRadarReadSettings********************/
            int taskRadarRS{0};
            bool TaskRadarReadSettings(int *_ptLine);
            /*********************TaskRadarWriteSettings********************/
            int taskRadarWS{0};
            bool TaskRadarWriteSettings(int *_ptLine);
        };
    }
}

extern Radar::iSys::iSys400xPower *isys400xpwr;
extern Radar::iSys::iSys400x *isys400x[2];
