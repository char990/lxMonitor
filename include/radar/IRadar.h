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
        uint8_t radarFlag{0};
        virtual bool TaskRadar() = 0;
        UciRadar &uciradar;
    protected:
        OprSp *oprSp{nullptr};
        RadarStatus radarStatus{RadarStatus::POWER_UP};
    };
}
