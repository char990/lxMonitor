/*
 *
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <mcheck.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <module/Epoll.h>
#include <module/SerialPort.h>
#include <module/TimerEvent.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <gpio/GpioOut.h>

#include <module/DebugConsole.h>

#include <radar/Monitor.h>

#include <websocket/WsServer.h>

#include <3rdparty/catch2/enable_test.h>

const char *FirmwareVer = "0100";
const char *CONFIG_PATH = "config";
const char *metapath = "/mnt/mmcblk0p1";
char *mainpath;

using namespace std;

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
    PrintDbg(DBG_LOG, "%s", buf);
    PrintDbg(DBG_LOG, "%s", sbuf);
    PrintDbg(DBG_LOG, "%s", buf);
}

// a wrapper for time()
time_t GetTime(time_t *t)
{
    return time(t);
}

bool ticktock = true;
class TickTock : public IPeriodicRun
{
public:
    TickTock()
    {
    }

    virtual void PeriodicRun() override
    {
        pPinHeartbeatLed->Toggle();
        pPinWdt->Toggle();
        if (ticktock)
        {
            PrintDbg(DBG_HB, "%c   \x08\x08\x08", s[cnt & 0x03]);
            fflush(stdout);
        }
    };

private:
    int cnt{0};
    const char s[4]{'-', '\\', '|', '/'};
};

void GpioInit()
{
    unsigned int pins[] = {
        PIN_CN9_7,
        PIN_CN9_8,
        PIN_CN9_9,
        PIN_CN9_10,
        PIN_CN7_1,
        PIN_CN7_2,
        PIN_CN7_3,
        PIN_CN7_4,
        PIN_CN7_7,
        PIN_CN7_8,
        PIN_CN7_9,
        PIN_CN7_10,
        PIN_IN1,
        PIN_IN2,
        PIN_IN3,
        PIN_IN4,
        PIN_IN5,
        PIN_IN6,
        PIN_IN7,
        PIN_IN8,
        PIN_CN9_2,
        PIN_CN9_4,
        PIN_HB_LED,
        PIN_ST_LED,
        PIN_RELAY_CTRL,
        PIN_WDT};

    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++)
    {
        GpioEx::Export(pins[i]);
    }
    Utils::Time::SleepMs(1000);                     // must sleep 1 second to wait "export"
    pPinHeartbeatLed = new GpioOut(PIN_HB_LED, 1);  // heartbeat led, yellow
    pPinStatusLed = new GpioOut(PIN_ST_LED, 1);     // status led, green
    pPinWdt = new GpioOut(PIN_WDT, 1);              // watchdog
    pPinRelay = new GpioOut(PIN_RELAY_CTRL, 0);     // relay off
    pPinMosfet1 = new GpioOut(PIN_MOSFET1_CTRL, 0); // mosfet off
    pPinMosfet2 = new GpioOut(PIN_MOSFET2_CTRL, 0); // mosfet off
}

#ifdef CATCH2TEST
int test_mask_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    // setenv("MALLOC_TRACE","./test.log",1);
    // mtrace();
    mainpath = get_current_dir_name();
    if (strlen(mainpath) > 64)
    {
        system("date >> exlog");
        system("echo 'mainpath is longer than 64 bytes.\n' >> exlog");
        printf("mainpath is longer than 64 bytes.\n");
        exit(1);
    }
    // --------------------------------------------------
    try
    {
        DbHelper::Instance().Init(CONFIG_PATH);
        PrintVersion(true);
        GpioInit();
        Epoll::Instance().Init(32);
        auto tmrEvt10ms = new TimerEvent{10, "[tmrEvt10ms]"};
        auto tmrEvt100ms = new TimerEvent{100, "[tmrEvt100ms]"};
        auto tmrEvt1Sec = new TimerEvent{1000, "[tmrEvt1Sec]"};
        auto pTickTock = new TickTock{};
        tmrEvt1Sec->Add(pTickTock);
        auto console = new DebugConsole();

        // monitor
        auto monitor1 = new Monitor(1);
        tmrEvt10ms->Add(monitor1);
        // auto monitor2 = new Monitor(2);
        // tmrEvt10ms->Add(monitor2);

        PrintDbg(DBG_LOG, ">>> DONE >>>");
        printf("\n=>Input '?<Enter>' to get console help.\n\n");
        /*************** Start ****************/
        while (1)
        {
            Epoll::Instance().EventsHandle();
        }
        /************* Never hit **************/
    }
    catch (const std::exception &e)
    {
        // muntrace();
        PrintDbg(DBG_LOG, "\n!!! main exception :%s", e.what());
        return 255;
        // clean
    }
    // muntrace();
}

// E-O-F
