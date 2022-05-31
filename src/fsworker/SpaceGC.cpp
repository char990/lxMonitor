#include <fsworker/SpaceGC.h>

#include <vector>
#include <string>
#include <algorithm>

#include <Consts.h>
#include <module/MyDbg.h>
#include <module/Files.h>
#include <module/Utils.h>

void SpaceGC::PeriodicRun()
{
    RefreshVfsSt();
    auto t = time(nullptr) / 60 * 60; // run GC  per minute
    if (t != ts)
    {
        ts = t;
        CollectGarbage();
    }
}

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
            RefreshVfsSt();
            auto apc = (unsigned long long)vfsSt.f_bavail * vfsSt.f_bsize;
            if (apc < GIGA(freeGiga))
            {
                if ((sdindex + 1) == subdir.size())
                { // this is the last directory, do not rm it. throw
                    throw std::runtime_error(FmtException("GC failed. No enough space in %s", metapath));
                }
                if (!bgc)
                {
                    bgc = true;
                    char buf[1024];
                    PrintStatVfs(buf);
                    Ldebug("'%s' GC starts: %s", metapath, buf);
                }
                Ldebug("Remove dir '%s'", subdir.at(sdindex).c_str());
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
                    Ldebug("'%s' GC finished: %s", metapath, buf);
                }
                return;
            }
        }
    }
}

int SpaceGC::PrintStatVfs(char *buf)
{
    int len;
    if ((vfsSt.f_bsize % MEGA(1)) == 0)
    {
        len = sprintf(buf, "BlockSize=%uM", vfsSt.f_bsize / MEGA(1));
    }
    else if ((vfsSt.f_bsize % KILO(1)) == 0)
    {
        len = sprintf(buf, "BlockSize=%uK", vfsSt.f_bsize / KILO(1));
    }
    else
    {
        len = sprintf(buf, "BlockSize=%u", vfsSt.f_bsize);
    }
    len += sprintf(buf + len, ", Total=%u, Used=%u, Avail=%u",
                   vfsSt.f_blocks, vfsSt.f_blocks - vfsSt.f_bavail, vfsSt.f_bavail);
    return len;
}

void SpaceGC::RefreshVfsSt()
{
    if (StatVfs() < 0)
    {
        char buf[256];
        sprintf(buf, "statvfs(%s) failed", metapath);
        Ldebug(buf);
        throw std::runtime_error(buf);
    }
}

#include <3rdparty/catch2/EnableTest.h>
#if 0 || _ENABLE_TEST_ == 0
int SpaceGC::StatVfs()
{
    return statvfs(metapath, &vfsSt);
}
#else
int SpaceGC::StatVfs()
{
    vfsSt.f_bsize = GIGA(1);
    vfsSt.f_blocks = 100;
    vfsSt.f_bavail = fake_bavail++;
    return 0;
}

#include <3rdparty/catch2/catch.hpp>
#define FREE_SPACE 4
TEST_CASE("Class SpaceGC", "[SpaceGC]")
{
    SpaceGC gc{FREE_SPACE};
    SECTION("CollectGarbage")
    {
        char cmd[64];
        int len = sprintf(cmd, "mkdir -p %s/20000101", metapath);
        for (char i = '1'; i < ('1' + FREE_SPACE); i++)
        {
            cmd[len - 1] = i;
            system(cmd);
        }
        gc.fake_bavail = 0;
        printf("Before GC:\n");
        Utils::Exec::Shell("ls -l %s", metapath);
        gc.CollectGarbage();
        printf("\nAfter GC:\n");
        Utils::Exec::Shell("ls -l %s", metapath);
    }
}
#endif
