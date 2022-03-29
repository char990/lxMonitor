#include <radar/IRadar.h>

using namespace Radar;

const uint8_t IRadar::RADAR_CONNECTED;

void IRadar::SetFlag(bool c, const uint8_t vb)
{
    if (c)
    {
        radarFlag |= vb;
    }
    else
    {
        radarFlag &= ~vb;
    }
}
