#include <cstdio>
#include <cstring>

#include <Consts.h>
#include <module/MyDbg.h>

#ifdef DEBUG
const char *MAKE = "Debug";
#else
const char *MAKE = "Release";
#endif

#ifndef __BUILDTIME__
#define __BUILDTIME__ (__DATE__ " " __TIME__ " UTC")
#endif

int PrintfVersion_(bool start, char *buf)
{
    return snprintf(buf, 63, "%s* %s ver:%s @ %s *",
                    start ? ">>> START >>> " : "",
                    MAKE, FirmwareVer, __BUILDTIME__); // __BUILDTIME__ is defined in Makefile
}

void PrintVersion(bool start)
{
    char sbuf[64];
    int len = PrintfVersion_(start, sbuf);
    char buf[64];
    memset(buf, '*', len);
    buf[len] = '\0';
    auto lvl = start ? DBG_LOG : DBG_PRT;
    PrintDbg(lvl, "%s", buf);
    PrintDbg(lvl, "%s", sbuf);
    PrintDbg(lvl, "%s", buf);
}

