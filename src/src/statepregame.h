#pragma once

#include "iplayerstate.h"
#include "game.h"

class StatePreGameParams:public IStateChangeParams
{
public:
    StatePreGameParams(Game *game);
    ~StatePreGameParams();

    Game *m_game;
};

class StatePreGame:public IPlayerState
{
public:
    StatePreGame();
    ~StatePreGame();

    uint8_t State() const;

    void StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params);
    bool HandleInput(const Input *input, const uint8_t playerindex);
    void Update(const int ticks, const uint8_t playerindex, Game *game);
    void Draw(const uint8_t playerindex);

private:
    Game *m_game;
};
