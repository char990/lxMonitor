#include <fsworker/SaveCSV.h>
#include <cstdio>
#include <memory>
#include <fcntl.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

#include <Consts.h>

#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;

const char * PHOTO_TAKEN = "Photo taken";

SaveCSV::SaveCSV(std::string filename)
    : filename(filename)
{
}

SaveCSV::~SaveCSV()
{
    if (csvfd > 0)
    {
        close(csvfd);
    }
}

int SaveCSV::SaveRadarMeta(struct timeval &time, const char *meta, const char *comment)
{
    char buf[32];
    Time::ParseTimeToLocalStr(&time, buf);
    if (memcmp(lastdate, buf, 10) != 0)
    {
        if (csvfd > 0)
        {
            close(csvfd);
        }
        memcpy(lastdate, buf, 10);
        lastdate[10] = 0;
        char date[9];
        date[0] = lastdate[6]; // year
        date[1] = lastdate[7];
        date[2] = lastdate[8];
        date[3] = lastdate[9];
        date[4] = lastdate[3]; // month
        date[5] = lastdate[4];
        date[6] = lastdate[0]; // date
        date[7] = lastdate[1];
        date[8] = 0;
        char csv[256];
        sprintf(csv, "mkdir -p %s/%s", metapath, date);
        csvfd = system(csv);
        if (csvfd != 0)
        {
            PrintDbg(DBG_LOG, "Can NOT mkdir '%s/%s'", metapath, date);
            csvfd = -1;
        }
        else
        {
            sprintf(csv, "%s/%s/%s.csv", metapath, date, filename.c_str());
            csvfd = open(csv, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
            if (csvfd < 0)
            {
                PrintDbg(DBG_LOG, "Can NOT open '%s'", csv);
            }
        }
    }
    if (csvfd > 0)
    {
        char xbuf[1024];
        int len;
        if (comment != nullptr)
        {
            len = snprintf(xbuf, 1023, "%s,%s,%s\n", buf, meta, comment);
        }
        else
        {
            len = snprintf(xbuf, 1023, "%s,%s\n", buf, meta);
        }
        write(csvfd, xbuf, len);
    }
    return 0;
}
