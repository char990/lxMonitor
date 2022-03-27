#pragma once


#include <cstdio>
#include <string>
#include <module/MyDbg.h>
class IGcEvent
{
public:
    IGcEvent(){};
    virtual ~IGcEvent(){};
    virtual void EventsHandle(uint32_t events)=0;

    int GetFd() { return eventFd; }

    virtual void UnknownEvents(const char *name, uint32_t events)
    {
        throw std::runtime_error(FmtException("[%s]Unkown events=0x%08X", name, events));
    }

    virtual void UnknownEvents(std::string name, uint32_t events)
    {
        throw std::runtime_error(FmtException("[%s]Unkown events=0x%08X", name.c_str(), events));
    }

    /// \brief  This is the events set to Epoll, could be used for DeleteEvent()
    uint32_t events{0};

    /// \brief  
    int eventFd{-1};
};
