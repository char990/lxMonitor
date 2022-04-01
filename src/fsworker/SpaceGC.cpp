#include <fsworker/SpaceGC.h>
#include <Consts.h>
#include <module/MyDbg.h>

#define SEC_PER_DAY (24 * 3600)

void SpaceGC::PeriodicRun()
{
    auto t = time(nullptr);
    t = t / SEC_PER_DAY * SEC_PER_DAY;
    if (t != ts)
    {
        ts = t;
        CollectGarbage();
    }
}

void SpaceGC::CollectGarbage()
{
    while(true)
    {
        if ((statvfs(metadevice, &fiData)) < 0)
        {
            char buf[256];
            sprintf(buf, "Failed to stat '%s'", metadevice);
            PrintDbg(DBG_LOG, buf);
            throw std::runtime_error(buf);
        }
        else
        {
            auto apc = fiData.f_bavail / (fiData.f_blocks / 100);


            if(apc>20)
            {
                PrintDbg(DBG_LOG, "'%s': BlockSize=%u, Total=%u, Used=%u(%u%%), Avail=%u(%u%%)", metadevice,
                        fiData.f_bsize, fiData.f_blocks,
                        fiData.f_blocks - fiData.f_bavail, 100 - apc,
                        fiData.f_bavail, apc);
                return;
            }
        }
    };
}
