#pragma once

#include "iplayerstate.h"
#include "game.h"

class StateMainMenuParams:public IStateChangeParams
{
public:
    StateMainMenuParams(Game *game);
    ~StateMainMenuParams();

    Game *m_game;
};

class StateMainMenu:public IPlayerState
{
public:
    StateMainMenu();
    ~StateMainMenu();

    uint8_t State() const;

    void StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params);
    bool HandleInput(const Input *input, const uint8_t playerindex);
    void Update(const int ticks, const uint8_t playerindex, Game *game);
    void Draw(const uint8_t playerindex);

private:
    uint8_t m_selected;
    Game *m_game;
};
