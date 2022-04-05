#pragma once

#include <string>

#include <radar/IRadar.h>
#include <uci/UciSettings.h>
#include <module/SerialPort.h>
#include <module/IOperator.h>
#include <module/BootTimer.h>
#include <fsworker/SaveCSV.h>

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
            int16_t range;  // m
            int16_t angle;  // deg
            int Print() { return printf("isys400x:%dDB, %dKM/H, %dM, angle=%dDEG\n", signal, speed, range, angle); }
            int Print(char *buf) { return sprintf(buf, "[S=%d V=%d R=%d A=%d]", signal, speed, range, angle); }
        };

        class TargetList
        {
        public:
            TargetList(std::string &name, int code) : name(name), code(code), csv(name + "Tlist"){};
            int flag{0}; // return of DecodeTargetFrame
            int cnt{0};
            Vehicle vehicles[MAX_TARGETS];

            /// \brief  decodes target list frame received from iSYS device - Note: only 16-bit.
            /// \return -1:Error; 0:NO target; 1-MAX:number of target
            int DecodeTargetFrame(uint8_t *packet, int packetLen);

            /// \brief For unit test
            void MakeTargetMsg(uint8_t *buf, int *len);

            int SaveTarget(const char *comment);

            int Print(char *buf);
            int Print();

        private:
            int code;
            bool hasVehicle{false};
            std::string &name;
            SaveCSV csv;
            struct timeval time
            {
                0, 0
            };
        };

        class iSys400x : public IRadar
        {
        public:
            iSys400x(UciRadar &uciradar);
            virtual ~iSys400x();

            virtual int RxCallback(uint8_t *buf, int len) override { return 0; }; // No RxCallback for iSYS

            bool TaskRadarPoll() override { return TaskRadarPoll_(&taskRadar_); };

            Vehicle *minRangeVehicle;

            int SaveTarget(const char *comment) { return targetlist.SaveTarget(comment); };

        protected:
            TargetList targetlist;

            BootTimer tmrTaskRadar;
            int taskRadar_;
            bool TaskRadarPoll_(int *_ptLine);

#define MAX_PACKET_SIZE (9 + MAX_TARGETS * 7 + 1)
            uint8_t packet[MAX_PACKET_SIZE];
            int packetLen;
            void SendSd2(const uint8_t *p, int len);
            void CmdStartAcquisition();
            void CmdStopAcquisition();
            void CmdReadTargetList();
            bool VerifyCmdAck();
            iSYS_Status ChkRxFrame(uint8_t *packet, int packetLen);

            /// \brief Read packet. If there are more than one packet in buf, only keep the last packet
            /// \return packet length
            int ReadPacket();

            void ClearRxBuf() { oprSp->rxRingBuf->Reset(); };
        };
    }
}
