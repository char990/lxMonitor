#pragma once

#include <string>
#include <module/IGcEvent.h>
#include <module/RingBuf.h>

#define OPTXBUF_SIZE 1024
class IOperator : public IGcEvent
{
public:
    IOperator() : IOperator(OPTXBUF_SIZE){};
    IOperator(int buf_size) : txRingBuf(buf_size){};
    virtual ~IOperator() {};
    bool IsTxRdy() { return txRingBuf.Cnt()==0 && txsize == 0; };

    int TxBytes(const uint8_t *data, int len);
    int TxHandle();
    void ClrTx();

protected:
    uint8_t optxbuf[OPTXBUF_SIZE];
    int bufsize;
    int txsize{0};
    int txcnt{0};
    RingBuf txRingBuf;
};
