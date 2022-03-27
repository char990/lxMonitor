#pragma once

#include <3rdparty/mongoose/mongoose.h>
#include <module/TimerEvent.h>

class ConnUri
{
public:
    struct mg_connection *c{nullptr};
    const char *uri{nullptr};
};

class WsServer : public IPeriodicRun
{
public:
    WsServer(int port, TimerEvent * tmrEvt);
    ~WsServer();

    virtual void PeriodicRun() override;

private:
    struct mg_mgr mgr; // Event manager
    static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
    TimerEvent * tmrEvt;
    static const char * WS;
    static const char * ECHO;
    static const char * REST;
//    static std::vector<ConnUri> conn;
//    static ConnUri * FindConn(unsigned long id);
//    static void PushConn(ConnUri & conn);
};
