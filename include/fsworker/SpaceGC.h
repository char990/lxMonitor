/* Disk space Garbage collector */
#pragma once

#include <time.h>
#include <sys/statvfs.h>

#include <module/IPeriodicRun.h>

class SpaceGC : public IPeriodicRun
{
public:
    SpaceGC(unsigned int freeGiga):freeGiga(freeGiga){};

    virtual void PeriodicRun() override;
    
    void CollectGarbage();

    int fake_bavail{0};        // this is for unit test. No use in real program

private:
    unsigned int freeGiga;

    time_t ts{0};

    struct statvfs vfsSt;

    int StatVfs();

    int PrintStatVfs(char *buf);

    void RefreshVfsSt();
};
