#pragma once


#include <module/SerialPort.h>
#include <module/IOperator.h>


/// \brief Rx callback
class IRxCallback
{
public:
    /// \brief When data received, call this callback
    virtual int RxCallback(uint8_t * buf, int len) = 0;
};

/// \brief  Operator from serial port
class OprSp : public IOperator
{
public:
    OprSp(uint8_t comX, int bps, IRxCallback * rxCallback);
    OprSp(uint8_t comX, int bps, int rxbufsize, IRxCallback * rxCallback);
    ~OprSp();

    const char* Name() { return sp->Config().name; };
    int ComX() { return comX; };
    int Bps() { return sp->Config().baudrate; };

    /// \brief  Called by upperLayer
    virtual bool IsTxReady();
    /// \brief  Called by upperLayer
    virtual int Tx(const uint8_t * data, int len);

    /// \brief  Called by Epoll, receiving & sending handle
    virtual void EventsHandle(uint32_t events) override;
    /*--------------------------------------------------------->*/

    int ReOpen();

    IRxCallback * rxCallback;
    RingBuf *rxRingBuf{nullptr};

private:
    uint8_t comX;
    SerialPort *sp;

    /// \brief  Called in EventsHandle
    int RxHandle();
};
