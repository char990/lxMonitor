// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved
//
// Example Websocket server. See https://mongoose.ws/tutorials/websocket-server/

#include <module/MyDbg.h>

#include <websocket/WsServer.h>

#include <uci/DbHelper.h>

unsigned int ws_hexdump=0;

const char *WsServer::WS = "/ws";
const char *WsServer::ECHO = "/echo";
const char *WsServer::REST = "/rest";

// This RESTful server implements the following endpoints:
//   /websocket - upgrade to Websocket, and implement websocket echo server
void WsServer::fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    if (ev == MG_EV_OPEN)
    {
        c->is_hexdumping = ws_hexdump;
    }
    else if (ev == MG_EV_HTTP_MSG)
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        if (mg_http_match_uri(hm, WS))
        {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Connected WS on %s, from %d.%d.%d.%d", WS, ip[0], ip[1], ip[2], ip[3]);
            c->fn_data = (void *)WS;
        }
        else if (mg_http_match_uri(hm, ECHO))
        {
            // Upgrade to websocket. From now on, a connection is a full-duplex
            // Websocket connection, which will receive MG_EV_WS_MSG events.
            mg_ws_upgrade(c, hm, NULL);
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Connected WS on %s, from %d.%d.%d.%d", ECHO, ip[0], ip[1], ip[2], ip[3]);
            c->fn_data = (void *)ECHO;
        }
        else if (mg_http_match_uri(hm, REST))
        {
            // Serve REST response
            mg_http_reply(c, 200, "", "{\"%s_output\": %d}\n", REST, 123456789);
            c->fn_data = (void *)REST;
        }
    }
    else if (ev == MG_EV_WS_MSG)
    {
        // Got websocket frame. Received data is wm->data. Echo it back!
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        printf("\n-----------------\n");
        for (int i = 0; i < wm->data.len; i++)
        {
            putchar(wm->data.ptr[i]);
        }
        printf("\n-----------------\n");
        if (wm->data.len > 0)
        {
            if (c->fn_data == (void *)ECHO)
            {
                mg_ws_send(c, ECHO, strlen(ECHO), WEBSOCKET_OP_TEXT);
                mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
            }
            else if (c->fn_data == (void *)WS)
            {
                mg_ws_send(c, WS, strlen(WS), WEBSOCKET_OP_TEXT);
                mg_ws_send(c, wm->data.ptr, wm->data.len, WEBSOCKET_OP_TEXT);
            }
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        if (c->fn_data == (void *)ECHO || c->fn_data == (void *)WS)
        {
            uint8_t *ip = (uint8_t *)&c->rem.ip;
            Ldebug("Disconnected WS on %s, from %d.%d.%d.%d", (const char *)c->fn_data, ip[0], ip[1], ip[2], ip[3]);
        }
    }
    (void)fn_data;
}

WsServer::WsServer(int port, TimerEvent *tmrEvt)
    : tmrEvt(tmrEvt)
{
    if (port < 1024 || port > 65535)
    {
        throw std::invalid_argument(FmtException("WsServer error: port: %d", port));
    }
    char buf[32];
    sprintf(buf, "ws://0.0.0.0:%d", port);
    mg_mgr_init(&mgr); // Initialise event manager
    Ldebug("Starting WS listener on %s%s", buf, WS);
    mg_http_listen(&mgr, buf, fn, NULL); // Create HTTP listener
    tmrEvt->Add(this);
}

WsServer::~WsServer()
{
    mg_mgr_free(&mgr);
    tmrEvt->Remove(this);
    tmrEvt = nullptr;
}

void WsServer::PeriodicRun()
{
    mg_mgr_poll(&mgr, 0); // Infinite event loop
}
