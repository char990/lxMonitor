#pragma once


#include <time.h>
#include <sys/timerfd.h>
#include <string>
#include <vector>

#include <module/IGcEvent.h>
#include <module/IPeriodicRun.h>

class TimerEvent : public IGcEvent
{
public:
    /// \brief		Constructor that sets up scheduler with period of ms.
    TimerEvent(int ms, std::string name);
    
    /// \brief		Destructor. Closes fd.
    ~TimerEvent();

    /// \brief      Event handle, will be called in Epoll when events rise
    void EventsHandle(uint32_t events) override;

    /// \brief      Add delegate IPeriodicRun
    void Add(IPeriodicRun * evt);

    /// \brief      Add delegate IPeriodicRun
    void Remove(IPeriodicRun * evt);

private:
    std::string name;
    //int ticks, sec, cnt;
    std::vector<IPeriodicRun *> pEvts;
};
