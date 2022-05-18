#pragma once

class Vdebug
{
public:
    virtual void SetVdebug(int v) { vdebug = v; };
    virtual int GetVdebug() { return vdebug; };

private:
    int vdebug{0};
};
