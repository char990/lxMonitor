#pragma once

#include <module/IGcEvent.h>

#define DC_INBUF_SIZE 256

class Command
{
public:
    const char * cmd;
    const char * help;
    int (*function)(int argc, char *argv[]);
};

class DebugConsole : public IGcEvent
{
public:
    DebugConsole();
    ~DebugConsole();
    virtual void EventsHandle(uint32_t events) override;
private:
    char inbuf[DC_INBUF_SIZE];
    int cnt{0};
    void Process();
    void WrongArg();
    int _fcntl;

    // command list
    static int Cmd_help(int argc, char *argv[]);
    static int Cmd_t(int argc, char *argv[]);
    static int Cmd_ver(int argc, char *argv[]);
    static int Cmd_ws(int argc, char *argv[]);
    static int Cmd_shoot(int argc, char *argv[]);
    static int Cmd_isys(int argc, char *argv[]);
    static int Cmd_stkr(int argc, char *argv[]);

    static const Command CMD_LIST[];
};
