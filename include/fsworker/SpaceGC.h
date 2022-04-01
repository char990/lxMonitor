/* Disk space Garbage collector */
#pragma once

#include <time.h>
#include <sys/statvfs.h>

#include <module/IPeriodicRun.h>

class SpaceGC : public IPeriodicRun
{
public:
    SpaceGC()
    {
        ts = time(nullptr);
        CollectGarbage();
    };
    virtual void PeriodicRun() override;

private:
    time_t ts;
    struct statvfs fiData;
    void CollectGarbage();
};
