#pragma once


class IPeriodicRun
{
public:
    virtual ~IPeriodicRun(){};

    virtual void PeriodicRun()=0;
};


