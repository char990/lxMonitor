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
        CollectGarbage();
    };
    virtual void PeriodicRun() override;

private:
    time_t ts{0};
    struct statvfs fiData;

    int StatVfs();

    void CollectGarbage();

    int PrintStatVfs(char *buf);
};
