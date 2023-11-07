#pragma once

#include <stdint.h>

class IStateChangeParams
{
public:
    IStateChangeParams()            { };
    virtual ~IStateChangeParams()   { };
};

class IState
{
public:
    IState()            { };
    virtual ~IState()   { };

    virtual void StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params)=0;
    virtual uint8_t State() const=0;
};
