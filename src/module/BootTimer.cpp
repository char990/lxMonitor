#include <module/BootTimer.h>

void BootTimer::Setms(long ms)
{
    if(ms<0||ms==LONG_MAX)
    {
        ns=0;
        sec=LONG_MAX;   // time_t is long
        return;
    }
    struct timespec _CLOCK_BOOTTIME;
    clock_gettime(CLOCK_BOOTTIME, &_CLOCK_BOOTTIME);
    ns = (ms%1000)*1000000 + _CLOCK_BOOTTIME.tv_nsec;
    sec = ms/1000 + _CLOCK_BOOTTIME.tv_sec;
    if(ns>1000000000)
    {
        ns-=1000000000;
        sec++;
    }
}

void BootTimer::Setus(long us)
{
    if(us<0||us==LONG_MAX)
    {
        ns=0;
        sec=LONG_MAX;   // time_t is long
        return;
    }
    struct timespec _CLOCK_BOOTTIME;
    clock_gettime(CLOCK_BOOTTIME, &_CLOCK_BOOTTIME);
    ns = (us%1000)*1000 + _CLOCK_BOOTTIME.tv_nsec;
    sec = us/1000000 + _CLOCK_BOOTTIME.tv_sec;
    if(ns>1000000000)
    {
        ns-=1000000000;
        sec++;
    }
}

bool BootTimer::IsExpired()
{
    if(sec==LONG_MAX) return false;
    struct timespec _CLOCK_BOOTTIME;
    clock_gettime(CLOCK_BOOTTIME, &_CLOCK_BOOTTIME);
    return ((_CLOCK_BOOTTIME.tv_sec>sec) ||
             (_CLOCK_BOOTTIME.tv_sec==sec && _CLOCK_BOOTTIME.tv_nsec>=ns));
}

void BootTimer::Clear()
{
    Setms(LONG_MAX);
}
