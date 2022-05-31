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

#include <Consts.h>
#include <Version.h>

#include <uci/DbHelper.h>

#include <module/Epoll.h>
#include <module/SerialPort.h>
#include <module/TimerEvent.h>
#include <module/Utils.h>
#include <gpio/GpioOut.h>
#include <gpio/GpioIn.h>

#include <module/DebugConsole.h>

#include <radar/iSys.h>
#include <radar/Stalker.h>
#include <fsworker/SpaceGC.h>
#include <controller/Controller.h>
#include <websocket/WsServer.h>

const char *mainpath;

using namespace std;

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
            PrintDbg(DBG_HB, "%c   \x08\x08\x08", s[++cnt & 0x03]);
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
        Utils::Time::SleepMs(100);                    // must sleep 100ms to wait "export"
    }
    Utils::Time::SleepMs(1000);                    // must sleep 1 second to wait "export"
    pPinHeartbeatLed = new GpioOut(PIN_HB_LED, 1); // heartbeat led, yellow
    pPinStatusLed = new GpioOut(PIN_ST_LED, 1);    // status led, green
    pPinWdt = new GpioOut(PIN_WDT, 1);             // watchdog

    pOutput[0] = new GpioOut(PIN_MAIN_FAILURE, 1);
    pOutput[1] = new GpioOut(PIN_BATTERY_LOW, 1);
    pOutput[2] = new GpioOut(PIN_BATTERY_OPEN, 1);
    
    pInput[0] = pPinIn1 = new GpioIn(5, 5, PIN_MSG3); // read in tmrEvt10ms
    pInput[1] = pPinIn2 = new GpioIn(5, 5, PIN_MSG4); // read in tmrEvt10ms
    pInput[2] = pPinIn3 = new GpioIn(5, 5, PIN_MSG5); // read in tmrEvt10ms

    pPinRelay = new GpioOut(PIN_RELAY_CTRL, RELAY_NC_ON);     // relay off
    Utils::Time::SleepMs(1000);                    // must sleep 1 second to GpioOut stable
}

#include <3rdparty/catch2/EnableTest.h>
#if _ENABLE_TEST_ == 1
#define CATCH_CONFIG_MAIN
#include <3rdparty/catch2/catch.hpp>
TEST_CASE("main init", "[main init]")
{
    mainpath = get_current_dir_name();
}
int test_mask_main(int argc, char *argv[])
{
#else
int main(int argc, char *argv[])
{
#endif
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
        PrintVersion(true);
        DbHelper::Instance().Init(CONFIG_PATH);

        GpioInit();
        Epoll::Instance().Init(32);
        auto tmrEvt10ms = new TimerEvent{10, "[tmrEvt10ms]"};
        auto tmrEvt100ms = new TimerEvent{100, "[tmrEvt100ms]"};
        auto tmrEvt1000ms = new TimerEvent{1000, "[tmrEvt1000ms]"};

        tmrEvt1000ms->Add(new TickTock());
        tmrEvt1000ms->Add(new SpaceGC(DbHelper::Instance().GetUciSettings().FreeGiga()));

        auto console = new DebugConsole();

        // add camera alarm pin to 10ms
        for (int i = 0; i < 3; i++)
        {
            tmrEvt10ms->Add(pInput[i]);
        }

        // Camera3
        for(int i=0;i<3;i++)
        {
            cameras[i] = new Camera(i+1);
            tmrEvt100ms->Add(cameras[i]);
        }

        isys400xpwr = new Radar::iSys::iSys400xPower();
        tmrEvt100ms->Add(isys400xpwr);

        for(int i=0;i<2;i++)
        {
            stalkerTSS2[i] = new Radar::Stalker::StalkerTSS2(DbHelper::Instance().GetUciSettings().uciStalker[i]);
            tmrEvt10ms->Add(stalkerTSS2[i]);
            isys400x[i] = new Radar::iSys::iSys400x(DbHelper::Instance().GetUciSettings().uciiSys[i]);
            tmrEvt10ms->Add(isys400x[i]);
        }

        for (int i = 0; i < 2; i++)
        {
            monitors[i] = new Monitor(i + 1, cameras, stalkerTSS2, isys400x);
        }

        // controller
        auto controller = new Controller();
        tmrEvt10ms->Add(controller);

        Ldebug(">>> DONE >>>");
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
        Ldebug("\n!!! main exception :%s", e.what());
        return 255;
        // clean
    }
    // muntrace();
}

// E-O-F
