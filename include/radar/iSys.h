#pragma once

#include <string>

#include <radar/IRadar.h>
#include <uci/UciSettings.h>
#include <module/SerialPort.h>
#include <module/IOperator.h>
#include <module/BootTimer.h>

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
        };

        class TargetList
        {
        public:
            TargetList(int code) : code(code){};
            uint32_t flag{0};
            int cnt{0};
            Vehicle vehicles[MAX_TARGETS];

            /// \brief  decodes target list frame received from iSYS device - Note: only 16-bit.
            /// \return -1:Error; 0:NO target; 1-MAX:number of target
            int DecodeTargetFrame(uint8_t *packet, int packetLen);

            /// \brief For unit test
            void MakeTargetMsg(uint8_t *buf, int *len);

        private:
            int code;
        };

        class iSys400x : public IRadar
        {
        public:
            iSys400x(UciRadar &uciradar);
            virtual ~iSys400x();

            virtual int RxCallback(uint8_t *buf, int len) override { return 0; }; // No RxCallback for iSYS

            bool TaskRadarPoll() override { return TaskRadarPoll_(&taskRadar_); };

            Vehicle *minRangeVehicle;

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

            int ReadPacket();

            /// \brief return false if radarlost
            bool GetTarget();
        };

    }
}
