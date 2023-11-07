#include "statemainmenu.h"
#include "wasm4.h"
#include "palette.h"
#include "game.h"
#include "font5x7.h"
#include "textprinter.h"
#include "statepregame.h"

StateMainMenuParams::StateMainMenuParams(Game *game):m_game(game)
{

}

StateMainMenuParams::~StateMainMenuParams()
{

}

StateMainMenu::StateMainMenu():m_game(nullptr),m_selected(0)
{

}

StateMainMenu::~StateMainMenu()
{

}

uint8_t StateMainMenu::State() const
{
    return Game::STATE_MAINMENU;
}

void StateMainMenu::StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params)
{
    if(params)
    {
        m_game=((StateMainMenuParams *)params)->m_game;
    }
}

bool StateMainMenu::HandleInput(const Input *input, const uint8_t playerindex)
{
    if(input->GamepadButtonPress(playerindex+1,BUTTON_1))
    {
        bool changestate=true;

        // only player 0 triggers setting up new game / loading game
        if(playerindex==0)
        {
            // start new game, or load existing saved game
            if(m_selected==0)
            {
                const uint64_t seed=(((uint64_t)*MOUSE_X << 32) | ((uint64_t)*MOUSE_Y << 16) | m_game->GetGameData().m_ticks);
                m_game->GetGameData().SetupNewGame(seed);
            }
            else
            {
                changestate=m_game->GetGameData().LoadGame();
            }
        }
        // make sure player 1 has selected something and started a game (player 1 not in startup or start menu)
        else
        {
            if(m_game->GetPlayerState(0)->State()==Game::STATE_STARTUP || m_game->GetPlayerState(0)->State()==Game::STATE_MAINMENU)
            {
                changestate=false;
            }
        }

        if(changestate)
        {
            StatePreGameParams *pgp=new StatePreGameParams(m_game);
            m_game->ChangeState(playerindex,Game::STATE_PREGAME,pgp);
        }
    }

    // only player 1 can move up/down
    if(playerindex==0)
    {
        if(input->GamepadButtonPress(1,BUTTON_UP) && m_selected>0)
        {
            m_selected=0;
        }
        else if(input->GamepadButtonPress(1,BUTTON_DOWN) && m_selected==0)
        {
            m_selected=1;
        }
    }
    

    return true;
}

void StateMainMenu::Update(const int ticks, const uint8_t playerindex, Game *game=nullptr)
{

}

void StateMainMenu::Draw(const uint8_t playerindex)
{
    TextPrinter tp;
    tp.SetCustomFont(&Font5x7::Instance());
    tp.PrintCentered("Mini Civ",SCREEN_SIZE/2,15,128,PALETTE_WHITE);

    if(playerindex==0)
    {
        tp.Print("New Game",55,60,20,(m_selected==0 ? PALETTE_CYAN : PALETTE_WHITE));
        tp.Print("Load Game",55,80,20,(m_selected==1 ? PALETTE_CYAN : PALETTE_WHITE));
    }
    else
    {
        if(m_game->GetPlayerState(0)->State()==Game::STATE_STARTUP || m_game->GetPlayerState(0)->State()==Game::STATE_MAINMENU)
        {
            tp.PrintCentered("Waiting for Player 1",SCREEN_SIZE/2,60,20,PALETTE_CYAN);
        }
        else
        {
            tp.PrintCentered("Join Game",SCREEN_SIZE/2,60,20,PALETTE_CYAN);
        }
    }

    tp.PrintWrapped(VERSION_STR,SCREEN_SIZE-40,SCREEN_SIZE-8,10,40,PALETTE_BROWN,TextPrinter::JUSTIFY_RIGHT);
}
