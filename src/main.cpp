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

#include <controller/Controller.h>
#include <radar/Monitor.h>
#include <fsworker/SpaceGC.h>

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
    }
    Utils::Time::SleepMs(1000);                    // must sleep 1 second to wait "export"
    pPinHeartbeatLed = new GpioOut(PIN_HB_LED, 1); // heartbeat led, yellow
    pPinStatusLed = new GpioOut(PIN_ST_LED, 1);    // status led, green
    pPinWdt = new GpioOut(PIN_WDT, 1);             // watchdog

    pOutput[0] /*VO2*/ = pPinMosfet2 = new GpioOut(PIN_MOSFET2_CTRL, 0); // mosfet off
    pOutput[1] /*VO1*/= pPinMosfet1 = new GpioOut(PIN_MOSFET1_CTRL, 0); // mosfet off
    pOutput[2] = pPinRelay = new GpioOut(PIN_RELAY_CTRL, 0);     // relay off

    pInput[0] = pPinIn1 = new GpioIn(5, 5, PIN_G1_MSG1); // read in tmrEvt10ms
    pInput[1] = pPinIn2 = new GpioIn(5, 5, PIN_G1_MSG2); // read in tmrEvt10ms
    pInput[2] = pPinIn3 = new GpioIn(5, 5, PIN_G1_AUTO); // read in tmrEvt10ms
}

#if 0

#include <sys/statvfs.h>
#include <stdio.h>

main() {
  int fd;
  struct statvfs buf;

  if (statvfs("/mnt/.", &buf) == -1)
    perror("statvfs() error");
  else {
    printf("each block is %d bytes big\n", fs,
           buf.f_bsize);
    printf("there are %d blocks available out of a total of %d\n",
           buf.f_bavail, buf.f_blocks);
    printf("in bytes, that's %.0f bytes free out of a total of %.0f\n"
           ((double)buf.f_bavail * buf.f_bsize),
           ((double)buf.f_blocks * buf.f_bsize));
  }
}
#endif
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

        // Controller
        auto controller = new Controller();
        tmrEvt1000ms->Add(controller);

        // Camera3
        camera1 = new Camera(1);
        tmrEvt100ms->Add(camera1);
        camera2 = new Camera(2);
        tmrEvt100ms->Add(camera2);
        camera3 = new Camera(3);
        tmrEvt100ms->Add(camera3);
        
        // monitor
        auto monitor1 = new Monitor(1, camera1, camera3);
        tmrEvt10ms->Add(monitor1);
        // auto monitor2 = new Monitor(2, camera3, nullptr);
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
