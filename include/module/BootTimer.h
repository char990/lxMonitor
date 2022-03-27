#pragma once

#include <limits.h>
#include <time.h>

class BootTimer
{
public:
    BootTimer(){};
    BootTimer(long ms) { Setms(ms); };

    /// \brief		Set timer in ms (1/1000 second)
    /// \param		ms (If ms<0, set as LONG_MAX)
    void Setms(long ms);

    /// \brief		Set timer in us (1/1000000 second)
    /// \param		us (If us<0, set as LONG_MAX)
    void Setus(long us);

    /// \brief		If the timer is expired
    bool IsExpired();

    /// \brief		Clear timer, will be never expired
    void Clear();

    /// \brief      Check if timer is clear(unuse)
    bool IsClear()
    {
        return ns == 0 && sec == LONG_MAX;
    }

private:
    time_t sec{LONG_MAX};
    __syscall_slong_t ns{0};
};
