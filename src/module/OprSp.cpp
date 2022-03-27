#include <unistd.h>
#include <module/MyDbg.h>
#include <module/OprSp.h>
#include <module/Epoll.h>
#include <uci/DbHelper.h>

#define SP_BUF_SIZE 4096

OprSp::OprSp(uint8_t comX, int bps, IRxCallback *rxCallback)
    : OprSp(comX, bps, SP_BUF_SIZE, rxCallback)
{
}

OprSp::OprSp(uint8_t comX, int bps, int rxbufsize, IRxCallback *rxCallback)
    : IOperator(rxbufsize), rxCallback(rxCallback)
{
    this->comX = comX;
    SpConfig &spCfg = gSpConfig[comX];
    spCfg.baudrate = bps;
    sp = new SerialPort(spCfg);
    if (sp->Open() < 0)
    {
        throw std::runtime_error(FmtException("Open %s failed", sp->Config().name));
    }
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = sp->GetFd();
    Epoll::Instance().AddEvent(this, events);
    if(rxCallback==nullptr)
    {
        rxRingBuf = new RingBuf(1024);
    }
}

OprSp::~OprSp()
{
    Epoll::Instance().DeleteEvent(this, events);
    sp->Close();
    delete sp;
    if(rxRingBuf!=nullptr)
    {
        delete rxRingBuf;
    }
}

/*< IOperator --------------------------------------------------*/

/// \brief  Called by upperLayer
bool OprSp::IsTxReady()
{
    return IsTxRdy();
}

/// \brief  Called by upperLayer
int OprSp::Tx(const uint8_t *data, int len)
{
    int x = TxBytes(data, len);
    if (x > 0)
    {
        x = x * 1000 * sp->Config().bytebits / sp->Config().baudrate; // get ms
    }
    return (x < 10 ? 10 : x);
}

/// \brief  Called by Eepoll, receiving & sending handle
void OprSp::EventsHandle(uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLRDHUP | EPOLLERR))
    {
        if (ReOpen() == -1)
        {
            throw std::runtime_error(FmtException("%s closed: events=0x%08X and reopen failed", sp->Config().name, events));
        }
    }
    else if (events & EPOLLIN)
    {
        RxHandle();
    }
    else if (events & EPOLLOUT)
    {
        TxHandle();
    }
    else
    {
        UnknownEvents(sp->Config().name, events);
    }
}

/// \brief  Called in EventsHandle
int OprSp::RxHandle()
{
    uint8_t buf[4096];
    int n = read(eventFd, buf, 4096);
    if (n > 0)
    {
        if (IsTxRdy()) // if tx is busy, discard this rx
        {
            if (rxCallback != nullptr)
            {
                rxCallback->RxCallback(buf, n);
            }
            else
            {
                rxRingBuf->Push(buf, n);
            }
        }
        else
        {
            PrintDbg(DBG_LOG, "%s:ComTx not ready", sp->Config().name);
        }
    }
    return 0;
}

int OprSp::ReOpen()
{
    Epoll::Instance().DeleteEvent(this, events);
    sp->Close();
    eventFd = -1;
    if (sp->Open() < 0)
    {
        return -1;
    }
    events = EPOLLIN | EPOLLRDHUP;
    eventFd = sp->GetFd();
    Epoll::Instance().AddEvent(this, events);
    return 0;
}
