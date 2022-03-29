#pragma once

#include <module/OprSp.h>
#include <uci/UciSettings.h>

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

    class IRadar : public IRxCallback
    {
    public:
        IRadar(UciRadar &uciradar) : uciradar(uciradar){};
        virtual int RxCallback(uint8_t *buf, int len) = 0;
        virtual RadarStatus GetStatus() { return radarStatus; };
        virtual void SetStatus(RadarStatus s) { radarStatus = s; };
        virtual bool TaskRadarPoll() = 0;
        UciRadar &uciradar;

        bool IsConnected() { return (radarFlag & RADAR_CONNECTED) != 0; };
        void Connected(bool c) { SetFlag(c, RADAR_CONNECTED); };

    protected:
        void SetFlag(bool c, const uint8_t vb);
        uint8_t radarFlag{0};
        static const uint8_t RADAR_CONNECTED{1 << 0};

        OprSp *oprSp{nullptr};
        RadarStatus radarStatus{RadarStatus::POWER_UP};
    };
}
