#include <fcntl.h>
#include <unistd.h>
#include <argz.h>
#include <cstdarg>
#include <cstring>
#include <module/DebugConsole.h>
#include <module/Epoll.h>
#include <camera/Camera.h>
#include <radar/iSys.h>
#include <radar/Stalker.h>
#include <controller/Monitor.h>


const Command DebugConsole::CMD_LIST[] = {
    {"?",
     "This help",
     DebugConsole::Cmd_help},
    {"t",
     "Set ticktock ON/OFF",
     DebugConsole::Cmd_t},
    {"ver",
     "Print version",
     DebugConsole::Cmd_ver},
    /*{"ws",
     "Set websocket hexdump ON/OFF",
     DebugConsole::Cmd_ws},*/
    {"cam",
     "Take photo by camera 1-3. Usage: cam 1-3\n"
     "\t| Set camera 1-3 debug. Usage: cam 1-3 X",
     DebugConsole::Cmd_cam},
    {"stkr",
     "Stalker 1|2 debug info X(0:Off, 1:Clos). Usage: stkr 1|2 X",
     DebugConsole::Cmd_stkr},
    {"isys",
     "iSys400x 1|2 debug info X(0:Off, 1:Clos, 2:Away, 3:Both). Usage: isys 1|2 X\n"
     "\t| iSys400x power on|off. Usage: isys on|off",
     DebugConsole::Cmd_isys},
    {"monitor",
     "monitor 1|2 debug info X. Usage: monitor 1|2 X",
     DebugConsole::Cmd_monitor},
};

DebugConsole::DebugConsole()
{
    // set stdin
    _fcntl = fcntl(0, F_GETFL);
    fcntl(0, F_SETFL, _fcntl | O_NONBLOCK);
    eventFd = 0;
    events = EPOLLIN;
    Epoll::Instance().AddEvent(this, events);
}

DebugConsole::~DebugConsole()
{
    Epoll::Instance().DeleteEvent(this, events);
    fcntl(0, F_SETFL, _fcntl);
}

void DebugConsole::EventsHandle(uint32_t events)
{
    if (events & EPOLLIN)
    {
        while (1)
        {
            char *p = inbuf + cnt;
            int numRead = read(0, p, DC_INBUF_SIZE - cnt - 1);
            if (numRead <= 0)
            {
                return;
            }
            cnt += numRead;
            if (cnt >= DC_INBUF_SIZE - 1)
            {
                cnt = 0;
            }
            else
            {
                char *pe = p + numRead;
                while (p < pe)
                {
                    if (*p == '\n')
                    {
                        *p = '\0';
                        Process();
                        cnt = 0; // clear inbuf
                    }
                    p++;
                }
            }
        }
    }
}

void DebugConsole::Process()
{
    int len = strlen(inbuf);
    if (len == 0 || len >= DC_INBUF_SIZE)
    {
        printf("\n=>");
        fflush(stdout);
        return;
    }
    char *argz;
    size_t argz_len;
    if (argz_create_sep(inbuf, ' ', &argz, &argz_len) != 0)
    {
        return;
    }
    if (argz == nullptr || argz_len == 0)
    {
        return;
    }
    int argc = argz_count(argz, argz_len);
    if (argc == 0)
    {
        return;
    }
    char **argv = new char *[argc + 1];
    argz_extract(argz, argz_len, argv);
    int i = 0;
    int j = sizeof(CMD_LIST) / sizeof(CMD_LIST[0]);
    do
    {
        if (strcmp(argv[0], CMD_LIST[i].cmd) == 0)
        {
            int x = (*CMD_LIST[i].function)(argc, argv);
            if (x == -1)
            {
                WrongArg();
            }
            break;
        }
        i++;
    } while (i < j);
    if (i == j)
    {
        printf("Unknown command\nPlease use command from the Command list:\n");
        Cmd_help(argc, argv);
    }
    printf("\n=>");
    fflush(stdout);
    free(argz);
    delete (argv);
}

void DebugConsole::WrongArg()
{
    printf("Wrong argument(s)\n");
}

int DebugConsole::Cmd_help(int argc, char *argv[])
{
    PrintDash('-');
    printf("CMD\t| Comments\n");
    PrintDash('-');
    for (int i = 0; i < sizeof(CMD_LIST) / sizeof(CMD_LIST[0]); i++)
    {
        printf("%s\t| %s\n", CMD_LIST[i].cmd, CMD_LIST[i].help);
    }
    PrintDash('-');
    return 0;
}

extern bool ticktock;
int DebugConsole::Cmd_t(int argc, char *argv[])
{
    printf("Ticktock %s\n", ticktock ? "OFF" : "ON");
    ticktock = !ticktock;
    return 0;
}

extern void PrintVersion(bool);
int DebugConsole::Cmd_ver(int argc, char *argv[])
{
    PrintVersion(false);
    return 0;
}

extern unsigned int ws_hexdump;
int DebugConsole::Cmd_ws(int argc, char *argv[])
{
    printf("Set websocket hexdump %s\n", ws_hexdump ? "OFF" : "ON");
    ws_hexdump = !ws_hexdump;
    return 0;
}

int DebugConsole::Cmd_cam(int argc, char *argv[])
{
    char c;
    if (argc >= 2)
    {
        if (sscanf(argv[1], "%c", &c) != 1 || c < '1' || c > '3')
        {
            return -1;
        }
    }
    if (argc == 2)
    {
        cameras[c - '1']->ConTakePhoto();
    }
    else if (argc == 3)
    {
        int x;
        if (sscanf(argv[2], "%d", &x) != 1)
        {
            return -1;
        }
        cameras[c - '1']->SetVdebug(x);
    }
    return 0;
}

int DebugConsole::Cmd_stkr(int argc, char *argv[])
{
    if (argc == 3)
    {
        int r = argv[1][0] - '1';
        int s = argv[2][0] - '0';
        if ((r == 0 || r == 1) && (s >= 0 && s <= 3))
        {
            stalkerTSS2[r]->SetVdebug(s);
            return 0;
        }
    }
    return -1;
}

int DebugConsole::Cmd_isys(int argc, char *argv[])
{
    if (argc == 2)
    {
        if (strcmp(argv[1], "on") == 0)
        {
            RelayNcOn();
            isys400xpwr->PwrOn();
            printf("isys1&2 ON\n");
            return 0;
        }
        else if (strcmp(argv[1], "off") == 0)
        {
            RelayNcOff();
            isys400xpwr->ManualOff();
            printf("isys1&2 OFF\n");
            return 0;
        }
    }
    else if (argc == 3)
    {
        int r = argv[1][0] - '1';
        int s = argv[2][0] - '0';
        if ((r == 0 || r == 1) && (s >= 0 && s <= 3))
        {
            isys400x[r]->SetVdebug(s);
            return 0;
        }
    }
    return -1;
}

int DebugConsole::Cmd_monitor(int argc, char *argv[])
{
    if (argc == 3)
    {
        int r = argv[1][0] - '1';
        int s = argv[2][0] - '0';
        if ((r == 0 || r == 1) && (s >= 0 && s <= 3))
        {
            monitors[r]->SetVdebug(s);
            return 0;
        }
    }
    return -1;
}
