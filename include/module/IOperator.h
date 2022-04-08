#pragma once

#include <string>
#include <module/IGcEvent.h>
#include <module/RingBuf.h>

#define OPTXBUF_SIZE 1024
class IOperator : public IGcEvent
{
public:
    IOperator() : IOperator(OPTXBUF_SIZE){};
    IOperator(int buf_size) : txbufsize(buf_size) , txRingBuf(buf_size)
    {
         txbuf = new uint8_t[txbufsize]; 
    };
    //    IOperator(int buf_size) : txRingBuf(buf_size){};

    virtual ~IOperator() { delete[] txbuf; };
    //    virtual ~IOperator() {};
    bool IsTxFree();

    int TxBytes(const uint8_t *data, int len);
    int TxHandle();
    void ClrTx();

protected:
    uint8_t optxbuf[OPTXBUF_SIZE];
    int bufsize;
    int txsize{0};
    int txcnt{0};
    RingBuf txRingBuf;

    int txbufsize;
    uint8_t *txbuf;
    uint8_t *ptx;
    int cnt{0};
};
