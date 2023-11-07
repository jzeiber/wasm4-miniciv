#include "statestartup.h"
#include "game.h"
#include "statemainmenu.h"

StateStartup::StateStartup()
{

}

StateStartup::~StateStartup()
{

}

uint8_t StateStartup::State() const
{
    return Game::STATE_STARTUP;
}

void StateStartup::StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params)
{

}

void StateStartup::Update(const int ticks, const uint8_t playerindex, Game *game)
{
    StateMainMenuParams *mmp=new StateMainMenuParams(game);
    game->ChangeState(playerindex,Game::STATE_MAINMENU,mmp);
}

bool StateStartup::HandleInput(const Input *i, const uint8_t playerindex)
{
    return true;
}

void StateStartup::Draw(const uint8_t playerindex)
{

}
