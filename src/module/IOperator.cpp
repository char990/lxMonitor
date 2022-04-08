#include <unistd.h>
#include <cstring>
#include <module/IOperator.h>
#include <module/Epoll.h>

int IOperator::TxBytes(const uint8_t *data, int len)
{
    if (len > (txbufsize - cnt) || len == 0 || data == nullptr)
    {
        return 0;
    }
    if (cnt == 0)
    {
        int n = write(eventFd, data, len);
        if (n < 0)
        {
            return -1;
        }
        else if (n < len)
        {
            cnt = len - n;
            ptx = txbuf;
            memcpy(txbuf, data + len, cnt);
            events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
            Epoll::Instance().ModifyEvent(this, events);
        }
    }
    else
    {
        auto ppush = ptx + cnt;
        if (ppush >= (txbuf + txbufsize))
        {
            ppush -= txbufsize;
        }
        int nb = txbuf + txbufsize - ppush; // left space to buf end
        if (len <= nb)
        {
            memcpy(ppush, data, len);
        }
        else
        {
            memcpy(ppush, data, nb);
            memcpy(txbuf, data + nb, len - nb);
        }
        cnt += len;
    }
    return len;

    /*************/
    if (txRingBuf.Vacancy() < len || len <= 0 || eventFd < 0)
    {
        return -1;
    }
    if (txRingBuf.Cnt() > 0 || txcnt < txsize)
    { // tx is busy
        txRingBuf.Push(data, len);
    }
    else
    {
        int n = write(eventFd, data, len);
        if (n < 0)
        {
            return -1;
        }
        else if (n < len)
        {
            txcnt = 0;
            int left = len - n;
            txsize = (left < OPTXBUF_SIZE) ? left : OPTXBUF_SIZE;
            left -= txsize;
            memcpy(optxbuf, data + n, txsize);
            if (left > 0)
            {
                txRingBuf.Push(data + n + txsize, left);
            }
            events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
            Epoll::Instance().ModifyEvent(this, events);
        }
    }
    return len;
}

void IOperator::ClrTx()
{
    txsize = 0;
    txcnt = 0;
    txRingBuf.Reset();
    events = EPOLLIN | EPOLLRDHUP;
    Epoll::Instance().ModifyEvent(this, events);
    cnt = 0;
}

int IOperator::TxHandle()
{
    if (cnt == 0)
    {
        ClrTx();
        return 0;
    }
    else
    {
        int txlen = txbuf + txbufsize - ptx; // ptx to buf end
        if (txlen > cnt)
        {
            txlen = cnt;
        }
        int n = write(eventFd, ptx, txlen);
        if (n < 0)
        {
            ClrTx();
            return -1;
        }
        cnt -= n;
        if (cnt > 0)
        {
            ptx += n;
            if (ptx >= (txbuf + txbufsize))
            {
                ptx -= txbufsize;
            }
        }
    }
    return cnt;
    /*************/
    int r = 0;
    if (txcnt == txsize)
    {
        if (txRingBuf.Cnt() == 0)
        {
            ClrTx();
            return 0;
        }
        else
        {
            txsize = txRingBuf.Pop(optxbuf, OPTXBUF_SIZE);
            txcnt = 0;
        }
    }

    int len = txsize - txcnt;
    int n = write(eventFd, optxbuf + txcnt, len);
    if (n < 0)
    {
        ClrTx();
        r = -1;
    }
    else if (n <= len)
    {
        txcnt += n;
        r = txsize - txcnt;
    }
    return r;
}

bool IOperator::IsTxFree()
{
//    return txRingBuf.Cnt() == 0 && txsize == 0;
    return txRingBuf.Cnt() == 0 && txsize == 0;
};
