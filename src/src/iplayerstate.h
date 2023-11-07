#pragma once

#include "istate.h"
#include "iupdatable.h"
#include "iinputhandler.h"
#include "idrawable.h"

class IPlayerState:public IState,public IUpdatable,public IInputHandler,public IDrawable
{
public:
    IPlayerState()                  { };
    virtual ~IPlayerState()         { };

    virtual void StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params)=0;
    virtual uint8_t State() const=0;

    virtual void Update(const int ticks, const uint8_t playerindex, Game *game=nullptr)=0;

    virtual bool HandleInput(const Input *i, const uint8_t playerindex)=0;

    virtual void Draw(const uint8_t playerindex)=0;
};
