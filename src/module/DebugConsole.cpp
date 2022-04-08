#include <fcntl.h>
#include <unistd.h>
#include <argz.h>
#include <cstdarg>
#include <cstring>
#include <module/DebugConsole.h>
#include <module/Epoll.h>
#include <camera/Camera.h>

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
    {"ws",
     "Set websocket hexdump ON/OFF",
     DebugConsole::Cmd_ws},
    {"shoot",
     "take photo by camera f|m|b",
     DebugConsole::Cmd_shoot},
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
            (*CMD_LIST[i].function)(argc, argv);
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

void DebugConsole::Cmd_help(int argc, char *argv[])
{
    PrintDash('-');
    printf("CMD\t| Comments\n");
    PrintDash('-');
    for (int i = 0; i < sizeof(CMD_LIST) / sizeof(CMD_LIST[0]); i++)
    {
        printf("%s\t| %s\n", CMD_LIST[i].cmd, CMD_LIST[i].help);
    }
    PrintDash('-');
}

extern bool ticktock;
void DebugConsole::Cmd_t(int argc, char *argv[])
{
    printf("Ticktock %s\n", ticktock ? "OFF" : "ON");
    ticktock = !ticktock;
}

extern void PrintVersion(bool);
void DebugConsole::Cmd_ver(int argc, char *argv[])
{
    PrintVersion(false);
}

extern unsigned int ws_hexdump;
void DebugConsole::Cmd_ws(int argc, char *argv[])
{
    printf("Set websocket hexdump %s\n", ws_hexdump ? "OFF" : "ON");
    ws_hexdump = !ws_hexdump;
}

void DebugConsole::Cmd_shoot(int argc, char *argv[])
{
    if (argc == 2)
    {
        char c;
        if (sscanf(argv[1], "%c", &c) == 1)
        {
            switch(c)
            {
                case 'f':
                camera1->TakePhoto();
                return;
                case 'm':
                camera3->TakePhoto();
                return;
                case 'b':
                camera2->TakePhoto();
                return;
            }
        }
    }
    printf("Wrong argument\nTry 'shoot f|m|b'\n");
}
