#include <fsworker/SpaceGC.h>

#include <vector>
#include <string>
#include <algorithm>

#include <Consts.h>
#include <module/MyDbg.h>
#include <module/Files.h>
#include <module/Utils.h>

#define SEC_PER_DAY (24 * 3600)

void SpaceGC::PeriodicRun()
{
    auto t = time(nullptr) / SEC_PER_DAY * SEC_PER_DAY;
    if (t != ts)
    {
        ts = t;
        CollectGarbage();
    }
}

#define GIGA(x) (x * 1024 * 1024 * 1024ULL)
#define MEGA(x) (x * 1024 * 1024)
#define KILO(x) (x * 1024)

#define FREE_SPACE 4

void SpaceGC::CollectGarbage()
{
    bool bgc = false;
    std::vector<std::string> subdir;
    int x = Files::GetDirs(metapath, subdir);
    if (x < 0)
    {
        throw std::runtime_error(FmtException("GetDirs(%s) failed", metapath));
    }
    if (subdir.size() > 0)
    {
        std::sort(subdir.begin(), subdir.end());
        for (int sdindex = 0; sdindex < subdir.size(); sdindex++)
        {
            if (StatVfs() < 0)
            {
                char buf[256];
                sprintf(buf, "statvfs(%s) failed", metadevice);
                PrintDbg(DBG_LOG, buf);
                throw std::runtime_error(buf);
            }
            else
            {
                auto apc = (unsigned long long)fiData.f_bavail * fiData.f_bsize;
                if (apc < GIGA(FREE_SPACE))
                {
                    if ((sdindex + 1) == subdir.size())
                    { // this is the last directory, do not rm it. throw
                        throw std::runtime_error(FmtException("GC failed. No enough space in %s", metadevice));
                    }
                    if (!bgc)
                    {
                        bgc = true;
                        char buf[1024];
                        PrintStatVfs(buf);
                        PrintDbg(DBG_LOG, "'%s' GC starts: %s", metadevice, buf);
                    }
                    PrintDbg(DBG_LOG, "Remove dir '%s'", subdir.at(sdindex).c_str());
                    if (Utils::Exec::Shell("rm -rf %s", subdir.at(sdindex).c_str()) != 0)
                    {
                        throw std::runtime_error(FmtException("rm -rf %s failed", subdir.at(sdindex).c_str()));
                    }
                }
                else
                {
                    if (bgc)
                    {
                        char buf[1024];
                        PrintStatVfs(buf);
                        PrintDbg(DBG_LOG, "'%s' GC finished: %s", metadevice, buf);
                    }
                    return;
                }
            }
        }
    }
}

int SpaceGC::PrintStatVfs(char *buf)
{
    int len;
    if ((fiData.f_bsize % MEGA(1)) == 0)
    {
        len = sprintf(buf, "BlockSize=%uM", fiData.f_bsize / MEGA(1));
    }
    else if ((fiData.f_bsize % KILO(1)) == 0)
    {
        len = sprintf(buf, "BlockSize=%uK", fiData.f_bsize / KILO(1));
    }
    else
    {
        len = sprintf(buf, "BlockSize=%u", fiData.f_bsize);
    }
    len += sprintf(buf + len, ", Total=%u, Used=%u, Avail=%u",
                   fiData.f_blocks, fiData.f_blocks - fiData.f_bavail, fiData.f_bavail);
    return len;
}

#include <3rdparty/catch2/EnableTest.h>
int SpaceGC::StatVfs()
{
#if _ENABLE_TEST_ == 1
    static int x = 0;
    x++;
    fiData.f_bsize = GIGA(1);
    fiData.f_blocks = 100;
    fiData.f_bavail = (x == 1) ? (FREE_SPACE+1) : x;
    return 0;
#else
    return statvfs(metadevice, &fiData);
#endif
}

#if 1
#if _ENABLE_TEST_ == 1
#include <3rdparty/catch2/catch.hpp>

TEST_CASE("Class SpaceGC", "[SpaceGC]")
{
    SpaceGC gc;
    SECTION("CollectGarbage")
    {
        char cmd[64];
        int len = sprintf(cmd, "mkdir -p %s/20000101", metapath);
        for (char i = '1'; i <= '5'; i++)
        {
            cmd[len - 1] = i;
            system(cmd);
        }
        Utils::Exec::Shell("ls -l %s", metapath);
        gc.PeriodicRun();
        Utils::Exec::Shell("ls -l %s", metapath);
    }
}
#endif
#endif