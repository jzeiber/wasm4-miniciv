#pragma once

#include "iplayerstate.h"

class StateStartup:public IPlayerState
{
public:
    StateStartup();
    ~StateStartup();

    uint8_t State() const;

    void StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params);
    void Update(const int ticks, const uint8_t playerindex, Game *game);

    bool HandleInput(const Input *i, const uint8_t playerindex);

    void Draw(const uint8_t playerindex);

};
