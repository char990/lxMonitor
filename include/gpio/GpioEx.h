#pragma once

class GpioEx
{
public:
    enum DIR
    {
        OUTPUT,
        INPUT
    };
    enum EDGE
    {
        NONE,
        RISING,
        FALLING,
        BOTHRF
    };
    GpioEx(unsigned int pin, DIR inout);
    GpioEx(unsigned int pin, EDGE edge);
    ~GpioEx(void);
    void SetDir(DIR inout);
    void SetValue(bool value);
    int GetValue(); // -1:falied, 0:low, 1:high
    void SetEdge(EDGE edge);
    unsigned int Pin() { return _pin; };
    static void Export(unsigned int pin);

private:
    int OpenFd(void);
    int GetFd(void);
    int CloseFd(void);
    unsigned int _pin;
    DIR _dir;
    EDGE _edge;
    int _fd;
    void Unexport();
};
