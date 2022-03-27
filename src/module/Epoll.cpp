#include <cstdio>
#include <unistd.h>
#include <module/MyDbg.h>
#include <module/Epoll.h>

void Epoll::Init(int max)
{
    if (max <= 0)
    {
        throw std::invalid_argument("Epoll size must be greater than 0");
    }
    if (MAX > 0)
    {
        throw std::runtime_error("Epoll Re-Init is not allowed");
    }
    MAX = max;
    epollfd = epoll_create(MAX);
    if (epollfd < 0)
    {
        throw std::runtime_error("Epoll create failed");
    }
    events = new epoll_event[MAX];
}

Epoll::~Epoll()
{
    if (events != nullptr)
    {
        delete[] events;
    }
    if (epollfd > 0)
        close(epollfd);
}

void Epoll::AddEvent(IGcEvent *event, uint32_t events)
{
    if (evtSize >= MAX)
    {
        throw std::overflow_error("Epoll overflow. Can't add event.");
    }
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = event;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, event->GetFd(), &ev);
    evtSize++;
}

void Epoll::DeleteEvent(IGcEvent *event, uint32_t events)
{
    if (evtSize > 0)
    {
        struct epoll_event ev;
        ev.events = events;
        ev.data.ptr = event;
        epoll_ctl(epollfd, EPOLL_CTL_DEL, event->GetFd(), &ev);
        evtSize--;
    }
}

void Epoll::ModifyEvent(IGcEvent *event, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = event;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, event->GetFd(), &ev);
}

void Epoll::EventsHandle()
{
    int num = epoll_wait(epollfd, events, MAX, -1);
    if (num == -1)
    {
        if (errno != EINTR)
        {
            throw std::runtime_error(FmtException("epoll_wait() error:%d", errno));
        }
    }
    for (int i = 0; i < num; i++)
    {
        IGcEvent *evt = (IGcEvent *)events[i].data.ptr;
        evt->EventsHandle(events[i].events);
    }
}
