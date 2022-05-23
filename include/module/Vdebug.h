#pragma once

class Vdebug
{
public:
    virtual void SetVdebug(int v) { vdebug = v; };
    virtual void SetVdebug(int x, int on) 
    {
        if(on)
        {
            vdebug |= (1<<x);
        }
        else
        {
            vdebug &= ~(1<<x);
        }
    };
    virtual int GetVdebug() { return vdebug; };

private:
    int vdebug{0};
};
