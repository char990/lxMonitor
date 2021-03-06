#pragma once

#include <module/OprSp.h>
#include <module/MyDbg.h>
#include <module/BootTimer.h>
#include <uci/UciSettings.h>
#include <module/Utils.h>
#include <module/Vdebug.h>

namespace Radar
{
    enum class RadarStatus
    {
        POWER_UP,
        INITIALIZING,
        READY,
        BUSY,
        NO_CONNECTION,
        EVENT
    };

    class IRadar : public IRxCallback, public Vdebug
    {
    public:
        IRadar(UciRadar &uciradar) : uciradar(uciradar) {};
        virtual int RxCallback(uint8_t *buf, int len) = 0;
        virtual RadarStatus GetStatus() { return radarStatus;};
        virtual void SetStatus(RadarStatus s) { radarStatus = s; };
        virtual bool TaskRadarPoll() = 0;
        UciRadar &uciradar;

        bool IsConnected() { return isConnected == Utils::STATE3::S3_1; };
        bool IsNotConnected() { return isConnected == Utils::STATE3::S3_0; };
        void Connected(bool c) { isConnected = c ? Utils::STATE3::S3_1 : Utils::STATE3::S3_0; };

    protected:
        void SetFlag(bool c, const uint8_t vb);
        uint8_t radarFlag{0};
        static const uint8_t RADAR_CONNECTED{1 << 0};

        Utils::STATE3 isConnected{Utils::STATE3::S3_NA};

        OprSp *oprSp{nullptr};
        RadarStatus radarStatus{RadarStatus::POWER_UP};
        BootTimer ssTimeout;
    };
}
