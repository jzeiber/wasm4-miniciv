#include "statepregame.h"
#include "stategame.h"
#include "textprinter.h"
#include "wasm4.h"
#include "palette.h"
#include "outputstringstream.h"
#include "font5x7.h"
#include "stringdata.h"

StatePreGameParams::StatePreGameParams(Game *game):m_game(game)
{

}

StatePreGameParams::~StatePreGameParams()
{

}

StatePreGame::StatePreGame():m_game(nullptr)
{

}

StatePreGame::~StatePreGame()
{

}

uint8_t StatePreGame::State() const
{
    return Game::STATE_PREGAME;
}

void StatePreGame::StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params)
{
    if(params)
    {
        m_game=((StatePreGameParams *)params)->m_game;
        m_game->GetGameData().m_playerready[playerindex]=false;     // reset ready for this player
    }
}

bool StatePreGame::HandleInput(const Input *input, const uint8_t playerindex)
{
    if(input->GamepadButtonPress(playerindex+1,BUTTON_UP) || input->GamepadButtonPress(playerindex+1,BUTTON_DOWN))
    {
        int8_t ci=m_game->GetGameData().GetCivIndexFromPlayerNum(playerindex+1);
        if(input->GamepadButtonPress(playerindex+1,BUTTON_UP))
        {
            ci=m_game->GetGameData().GetNextFreeCivIndex(ci,-1);
        }
        if(input->GamepadButtonPress(playerindex+1,BUTTON_DOWN))
        {
            ci=m_game->GetGameData().GetNextFreeCivIndex(ci,1);
        }
        m_game->GetGameData().AssignPlayerNumCivIndex(playerindex+1,ci);
        m_game->GetGameData().m_playerready[playerindex]=false;
    }

    if(input->GamepadButtonPress(playerindex+1,BUTTON_2))
    {
        m_game->GetGameData().m_playerready[playerindex]=false;
    }
    if(input->GamepadButtonPress(playerindex+1,BUTTON_1))
    {
        // only set player ready if a civ has been selected
        if(m_game->GetGameData().GetCivIndexFromPlayerNum(playerindex+1)>=0)
        {
            m_game->GetGameData().m_playerready[playerindex]=true;
        }
    }

    return true;
}

void StatePreGame::Update(const int ticks, const uint8_t playerindex, Game *game=nullptr)
{
    // change state when all players are ready (New player may join when game is already in progress by replacing CPU player - this should be fine)
    if(m_game->GetGameData().AllPlayersReady())
    {
        StateGameParams *gp=new StateGameParams(game);
        m_game->ChangeState(playerindex,Game::STATE_GAME,gp);
    }
}

void StatePreGame::Draw(const uint8_t playerindex)
{
    TextPrinter tp;
    tp.SetCustomFont(&Font5x7::Instance());
    tp.PrintCentered("Select Your Civilization",SCREEN_SIZE/2,15,128,PALETTE_WHITE);

    OutputStringStream ostr;

    for(int i=0; i<MAX_CIVILIZATIONS; i++)
    {
        tp.Print(civname[i],10,40+(15*i),100,PALETTE_WHITE);

        const uint8_t pn=m_game->GetGameData().m_civplayernum[i];

        if(pn==0)
        {
            tp.Print("CPU",80,40+(15*i),100,PALETTE_WHITE);
        }
        else if(pn==m_game->PlayerIndex()+1)
        {
            tp.Print("You",80,40+(15*i),100,m_game->GetGameData().m_playerready[pn-1] ? PALETTE_CYAN : PALETTE_BROWN);
        }
        else
        {
            ostr.Clear();
            ostr << "Player " << pn;
            tp.Print(ostr.Buffer(),80,40+(15*i),100,m_game->GetGameData().m_playerready[pn-1] ? PALETTE_CYAN : PALETTE_BROWN);
        }

    }

    if(m_game->GetGameData().m_playerready[m_game->PlayerIndex()])
    {
        tp.PrintCentered("Ready",SCREEN_SIZE/2,120,5,PALETTE_CYAN);
    }
    else
    {
        tp.PrintCentered("Press Button 1 When Ready",SCREEN_SIZE/2,120,100,PALETTE_BROWN);
    }

    if(m_game->GetGameData().AllPlayersReady()==true)
    {
        tp.PrintCentered("All Ready",SCREEN_SIZE/2,140,100,PALETTE_WHITE);
    }

}
