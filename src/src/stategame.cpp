#include "stategame.h"
#include "textprinter.h"
#include "font5x7.h"
#include "wasm4.h"
#include "wasm4draw.h"
#include "sprites.h"
#include "outputstringstream.h"
#include "palette.h"
#include "cppfuncs.h"
#include "icondata.h"
#include "unitdata.h"
#include "stringdata.h"
#include "improvementdata.h"

StateGameParams::StateGameParams(Game *game):m_game(game)
{

}

StateGameParams::~StateGameParams()
{

}

StateGame::StateGame(Map *map):m_game(nullptr),m_map(map),m_showinfo(false),m_mapx(0),m_mapy(0),m_scrollticks(0),m_blinkticks(0),m_view(0),m_menuidx(-1),m_submenuidx(-1),m_submenuidx2(-1),m_selecttype(SELECT_NONE),m_selectidx(-1),m_lastunitidx(-1),
m_availableicons{ICON_NONE}
{

}

StateGame::~StateGame()
{

}

uint8_t StateGame::State() const
{
    return Game::STATE_PREGAME;
}

void StateGame::StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params)
{
    if(params)
    {
        m_game=((StateGameParams *)params)->m_game;
        // if we just joined and it's this player's turn, then reset the turn ticks
        if(m_game->IsPlayerTurn(playerindex))
        {
            m_game->GetGameData().m_turnstarttick=m_game->GetGameData().m_ticks;
        }
        // game has now started since at least 1 player is in the game state
		m_game->GetGameData().m_gamestarted=true;
        // set map position to a unit or city if no units alive
        for(size_t i=0; i<countof(m_game->GetGameData().m_unit) && m_mapx==0 && m_mapy==0; i++)
        {
            if((m_game->GetGameData().m_unit[i].flags & UNIT_ALIVE)==UNIT_ALIVE && m_game->GetGameData().m_unit[i].owner==m_game->PlayerCivIndex(playerindex))
            {
                m_mapx=m_game->GetGameData().m_unit[i].x;
                m_mapy=m_game->GetGameData().m_unit[i].y;
            }
        }
        for(size_t i=0; i<countof(m_game->GetGameData().m_city) && m_mapx==0 && m_mapy==0; i++)
        {
            if(m_game->GetGameData().m_city[i].population>0 && m_game->GetGameData().m_city[i].owner==m_game->PlayerCivIndex(playerindex))
            {
                m_mapx=m_game->GetGameData().m_city[i].x;
                m_mapy=m_game->GetGameData().m_city[i].y;
            }
        }
    }
}

bool StateGame::HandleInput(const Input *input, const uint8_t playerindex)
{
    bool scrolled=false;
    const int8_t civindex=m_game->PlayerCivIndex(playerindex);

    // scroll around the map
    if(m_selecttype==SELECT_NONE && (m_view==VIEW_NONE || m_view==VIEW_MAP))
    {
        if(input->GamepadButtonDown(playerindex+1,BUTTON_LEFT)==true && m_scrollticks==0)
        {
            m_mapx--;
            scrolled=true;
        }
        if(input->GamepadButtonDown(playerindex+1,BUTTON_RIGHT)==true && m_scrollticks==0)
        {
            m_mapx++;
            scrolled=true;
        }
        if(input->GamepadButtonDown(playerindex+1,BUTTON_UP)==true && m_scrollticks==0)
        {
            if(m_mapy>0)
            {
            m_mapy--;
            scrolled=true;
            }
        }
        if(input->GamepadButtonDown(playerindex+1,BUTTON_DOWN)==true && m_scrollticks==0)
        {
            if(m_mapy<m_map->Height()-1)
            {
                m_mapy++;
                scrolled=true;
            }
        }
        MapCoord mc(m_map->Width(),m_map->Height(),m_mapx,m_mapy);
        m_mapx=mc.X();
        m_mapy=mc.Y();
    }
    // move selected unit
    else if(m_selecttype==SELECT_UNIT && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_unit) && m_game->IsPlayerTurn(playerindex))
    {
        int32_t dx=0;
        int32_t dy=0;
        if(input->GamepadButtonPress(playerindex+1,BUTTON_UP)==true)
        {
            dy-=1;
        }
        if(input->GamepadButtonPress(playerindex+1,BUTTON_RIGHT)==true)
        {
            dx+=1;
        }
        if(input->GamepadButtonPress(playerindex+1,BUTTON_DOWN)==true)
        {
            dy+=1;
        }
        if(input->GamepadButtonPress(playerindex+1,BUTTON_LEFT)==true)
        {
            dx-=1;
        }

        if(dx!=0 || dy!=0)
        {
            if(m_game->MoveUnit(m_game->PlayerCivIndex(playerindex),m_selectidx,dx,dy)==true)
            {
                // need to use unit coords in case it didn't move (attack multiple enemies)
                Unit *u=&(m_game->GetGameData().m_unit[m_selectidx]);
                if((u->flags & UNIT_ALIVE) == UNIT_ALIVE)
                {
                    MapCoord mc(m_map->Width(),m_map->Height(),u->x,u->y);
                    m_mapx=mc.X();
                    m_mapy=mc.Y();
                    m_blinkticks=0;
                }
            }
            // if unit died then set fixed loc so map doesn't scroll around
            if((m_game->GetGameData().m_unit[m_selectidx].flags & UNIT_ALIVE) != UNIT_ALIVE)
            {
                m_selecttype=SELECT_FIXEDLOC;
            }
        }
    }
    else if(m_view==VIEW_CIVDATA)
    {
        if(input->GamepadButtonPress(playerindex+1,BUTTON_UP) && m_submenuidx>0)
        {
            m_submenuidx--;
        }
        if(input->GamepadButtonPress(playerindex+1,BUTTON_DOWN) && m_submenuidx<(m_selectidx-5))
        {
            m_submenuidx++;
        }
    }
    else if(m_view==VIEW_CITYDETAIL)
    {
        if(input->GamepadButtonPress(playerindex+1,BUTTON_RIGHT) && m_submenuidx<=0)
        {
            m_submenuidx++;
        }
        if(input->GamepadButtonPress(playerindex+1,BUTTON_LEFT) && m_submenuidx==1)
        {
            m_submenuidx--;
        }
        if(input->GamepadButtonPress(playerindex+1,BUTTON_DOWN))
        {
            while(++m_submenuidx2<IMPROVEMENT_MAX && !(m_game->GetGameData().m_city[m_selectidx].improvements & (0x01 << m_submenuidx2)))
            {
            }
            m_submenuidx2=(m_submenuidx2==IMPROVEMENT_MAX ? -1 : m_submenuidx2);
        }
        if(input->GamepadButtonPress(playerindex+1,BUTTON_UP))
        {
            while(--m_submenuidx2>-1 && !(m_game->GetGameData().m_city[m_selectidx].improvements & (0x01 << m_submenuidx2)))
            {
            }
        }
    }

    if(input->GamepadButtonPress(playerindex+1,BUTTON_1)==true && m_menuidx>=0 && m_menuidx<countof(m_availableicons) && m_availableicons[m_menuidx]!=ICON_NONE)
    {
        // take action
        switch(m_availableicons[m_menuidx])
        {
        case ICON_NEXTUNIT:
        {
            int32_t idx=m_game->NextUnitIndex(civindex,(m_selecttype==SELECT_UNIT ? m_selectidx : m_lastunitidx),true);
            if(idx>=0)
            {
                m_selecttype=SELECT_UNIT;
                m_selectidx=idx;
                m_lastunitidx=idx;
                m_mapx=m_game->GetGameData().m_unit[m_selectidx].x;
                m_mapy=m_game->GetGameData().m_unit[m_selectidx].y;
                m_blinkticks=0;
            }
            break;
        }
        case ICON_NEXTLOCUNIT:
        {
            int32_t idx=m_game->NextUnitAtLocIndex(civindex,m_mapx,m_mapy,(m_selecttype==SELECT_UNIT ? m_selectidx : m_lastunitidx),false);
            if(idx>=0)
            {
                m_selecttype=SELECT_UNIT;
                m_selectidx=idx;
                m_lastunitidx=idx;
                m_blinkticks=0;
            }
            break;
        }
        case ICON_NEXTCITY:
        {
            int32_t idx=m_game->NextCityIndex(civindex,(m_selecttype==SELECT_CITY ? m_selectidx : -1));
            if(idx>=0)
            {
                m_selecttype=SELECT_CITY;
                m_selectidx=idx;
                m_mapx=m_game->GetGameData().m_city[m_selectidx].x;
                m_mapy=m_game->GetGameData().m_city[m_selectidx].y;
                m_submenuidx=0;
                m_submenuidx2=-1;
            }
            break;
        }
        case ICON_SCROLLMAP:
        {
            m_selecttype=SELECT_NONE;
            m_selectidx=-1;
            break;
        }
        case ICON_TOGGLEINFO:
        {
            m_showinfo=!m_showinfo;
            break;
        }
        case ICON_VIEWMAP:
        {
            m_view=VIEW_MAP;
            m_menuidx=0;               // only close button available, so have it selected
            m_selecttype=SELECT_NONE;
            break;
        }
        case ICON_CLOSE:
        {
            m_view=VIEW_NONE;
            m_menuidx=-1;
            m_selecttype=SELECT_NONE;
            break;
        }
        case ICON_CIVDATA:
        {
            m_view=VIEW_CIVDATA;
            m_menuidx=0;                // only close button available, so have it selected
            m_selecttype=SELECT_NONE;
            m_submenuidx=0;
            m_selectidx=0;
            break;
        }
        case ICON_ENDTURN:
        {
            m_game->EndPlayerTurn(civindex);
            m_view=VIEW_NONE;
            m_menuidx=0;        // default to select next unit after turn
            m_selecttype=SELECT_NONE;
            break;
        }
        case ICON_FOUNDCITY:
        {
            if(m_game->FoundCity(civindex,m_selectidx))
            {
                m_view=VIEW_NONE;
                m_menuidx=-1;
                m_selecttype=SELECT_NONE;
            }
            break;
        }
        case ICON_EXPANDCITY:
        {
            if(m_game->ExpandCity(playerindex,m_selectidx))
            {
                m_view=VIEW_NONE;
                m_menuidx=-1;
                m_selecttype=SELECT_NONE;
            }
            break;
        }
        case ICON_DISBAND:
        {
            if(m_game->DisbandUnit(playerindex,m_selectidx,false))
            {
                m_view=VIEW_NONE;
                m_menuidx=-1;
                m_selecttype=SELECT_NONE;
            }
            break;
        }
        case ICON_TOGGLESENTRY:
        {
            // toggle sentry for selected unit
            if(m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_unit))
            {
                m_game->GetGameData().m_unit[m_selectidx].flags^=UNIT_SENTRY;
            }
            break;
        }
        case ICON_ENTERCITY:
        {
            m_selecttype=SELECT_CITY;
            m_selectidx=m_game->CityIndexAtLocation(m_mapx,m_mapy);
            m_submenuidx=0;
            m_submenuidx2=-1;
            m_view=VIEW_CITYDETAIL;
            m_menuidx=0;        // 1st option is next city, so select it by default
            break;
        }
        case ICON_CHANGEBUILD:
        {
            if(m_selecttype==SELECT_CITY && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_city))
            {
                City *c=&(m_game->GetGameData().m_city[m_selectidx]);
                do
                {
                    c->producing++;
                    // make sure we dont' already have the improvement
                }while(c->producing<BUILDING_MAX && buildingxref[c->producing].buildingtype==BUILDINGTYPE_IMPROVEMENT && (c->improvements & (0x01 << buildingxref[c->producing].building))==(0x01 << buildingxref[c->producing].building));
                
                if(m_game->GetGameData().m_city[m_selectidx].producing>=BUILDING_MAX)
                {
                    m_game->GetGameData().m_city[m_selectidx].producing=0;
                }
            }
            break;
        }
        case ICON_BUYBUILD:
        {
            if(m_selecttype==SELECT_CITY && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_city))
            {
                m_game->CityBuyProducing(m_selectidx);
            }
            break;
        }
        case ICON_SELLIMPROVEMENT:
        {
            if(m_selecttype==SELECT_CITY && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_city) && m_submenuidx2>=0 && (m_game->GetGameData().m_city[m_selectidx].improvements & (0x01 << m_submenuidx2)))
            {
                // sell improvement m_submenuidx2
                m_game->GetGameData().m_city[m_selectidx].improvements=m_game->GetGameData().m_city[m_selectidx].improvements & (~(0x01 << m_submenuidx2));
                m_game->GetGameData().m_civ[m_game->GetGameData().m_city[m_selectidx].owner].gold+=improvementdata[m_submenuidx2].sellgold;
                m_submenuidx2=-1;
            }
            break;
        }
        default:
        {
            trace("Unknown Icon");
            break;
        }
        }
    }
    if(input->GamepadButtonPress(playerindex+1,BUTTON_2)==true)
    {
        if(m_menuidx<0)
        {
            m_menuidx=0;
        }
        else
        {
            m_menuidx++;
            if(m_menuidx>=countof(m_availableicons) || m_availableicons[m_menuidx]==ICON_NONE)
            {
                m_menuidx=0;
            }
        }
        if(m_availableicons[m_menuidx]==ICON_NONE)
        {
            m_menuidx=-1;
        }
    }

    if(scrolled==true)
    {
        m_scrollticks=5;
    }

    return true;
}

void StateGame::Update(const int ticks, const uint8_t playerindex, Game *game=nullptr)
{
    for(int32_t i=0; i<countof(m_availableicons); i++)
    {
        m_availableicons[i]=ICON_NONE;
    }
    // get available icons
    if(m_view==VIEW_NONE)
    {
        int8_t idx=0;
        m_availableicons[idx++]=ICON_NEXTUNIT;
        m_availableicons[idx++]=ICON_NEXTCITY;
        if(m_game->UnitIndexAtLocation(m_game->PlayerCivIndex(playerindex),m_mapx,m_mapy)>=0)
        {
            m_availableicons[idx++]=ICON_NEXTLOCUNIT;
        }
        if(m_selecttype!=SELECT_NONE)
        {
            m_availableicons[idx++]=ICON_SCROLLMAP;
        }
        m_availableicons[idx++]=ICON_TOGGLEINFO;
        m_availableicons[idx++]=ICON_VIEWMAP;

        if(m_selecttype==SELECT_NONE)
        {
            m_availableicons[idx++]=ICON_CIVDATA;
        }
        else if(m_selecttype==SELECT_UNIT && m_game->IsPlayerTurn(playerindex))
        {
            m_availableicons[idx++]=ICON_DISBAND;
        }

        if(m_selecttype==SELECT_UNIT && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_unit) && m_game->GetGameData().m_unit[m_selectidx].type!=UNITTYPE_SETTLER)
        {
            m_availableicons[idx++]=ICON_TOGGLESENTRY;
        }

        const int32_t ci=m_game->CityIndexAtLocation(m_mapx,m_mapy);
        if(ci>=0 && ci<countof(m_game->GetGameData().m_city) && m_game->GetGameData().m_city[ci].owner==m_game->PlayerCivIndex(playerindex))
        {
            m_availableicons[idx++]=ICON_ENTERCITY;
        }

        if(m_game->IsPlayerTurn(playerindex))
        {
            if(m_selecttype==SELECT_UNIT && m_game->CanFoundCity(m_game->PlayerCivIndex(playerindex),m_selectidx)==true)
            {
                m_availableicons[idx++]=ICON_FOUNDCITY;
            }
            else if(m_selecttype==SELECT_UNIT && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_unit) && m_game->GetGameData().m_unit[m_selectidx].type==UNITTYPE_SETTLER && m_game->GetGameData().m_unit[m_selectidx].movesleft>0)
            {
                if(m_game->CityCanExpand(m_game->CityIndexAtLocation(m_game->GetGameData().m_unit[m_selectidx].x,m_game->GetGameData().m_unit[m_selectidx].y))==true)
                {
                    m_availableicons[idx++]=ICON_EXPANDCITY;
                }
            }
 
            m_availableicons[idx++]=ICON_ENDTURN;
        }
    }
    if(m_view==VIEW_MAP)
    {
        m_availableicons[0]=ICON_CLOSE;
    }
    if(m_view==VIEW_CIVDATA)
    {
        int8_t idx=0;

        m_availableicons[idx++]=ICON_CLOSE;
    }
    if(m_view==VIEW_CITYDETAIL)
    {
        int8_t idx=0;

        m_availableicons[idx++]=ICON_NEXTCITY;

        int32_t improvementresources=0;
        if(m_game->GetGameData().m_city[m_selectidx].producing!=0)
        {
            if(buildingxref[m_game->GetGameData().m_city[m_selectidx].producing].buildingtype==BUILDINGTYPE_UNIT)
            {
                improvementresources=unitdata[buildingxref[m_game->GetGameData().m_city[m_selectidx].producing].building].buildresources;
            }
            else if(buildingxref[m_game->GetGameData().m_city[m_selectidx].producing].buildingtype==BUILDINGTYPE_IMPROVEMENT)
            {
                improvementresources=improvementdata[buildingxref[m_game->GetGameData().m_city[m_selectidx].producing].building].buildresources;
            }
        }

        // only show change build icon if we're producing a unit and the resources we have stored is != resources needed
        // otherwise if stored=needed then we must have just bought the improvement and have to wait for it to be built next turn
        if(m_game->GetGameData().m_city[m_selectidx].producing==0 || improvementresources!=m_game->GetGameData().m_city[m_selectidx].shields)
        {
            m_availableicons[idx++]=ICON_CHANGEBUILD;
        }

        if(m_game->GetGameData().m_city[m_selectidx].producing!=0 && m_game->IsPlayerTurn(playerindex))
        {
            m_availableicons[idx++]=ICON_BUYBUILD;
        }

        if(m_submenuidx2>=0)
        {
            m_availableicons[idx++]=ICON_SELLIMPROVEMENT;
        }

        // TODO - handle selling improvement

        m_availableicons[idx++]=ICON_CLOSE;
    }

    if(m_scrollticks)
    {
        m_scrollticks-=ticks;
    }
    m_blinkticks+=ticks;

    // another player may have killed selected unit on their turn, so check if the selected unit is dead and deselect
    if(m_selecttype==SELECT_UNIT && (m_selectidx<0 || m_selectidx>=countof(m_game->GetGameData().m_unit) || (m_game->GetGameData().m_unit[m_selectidx].flags & UNIT_ALIVE) != UNIT_ALIVE))
    {
        m_selecttype=SELECT_NONE;
        m_selectidx=-1;
    }

    // TODO - another player may have destroyed or taken over select city, so check for that and deselect
}

void StateGame::Draw(const uint8_t playerindex)
{
    switch(m_view)
    {
    case VIEW_MAP:
        DrawMap(playerindex);
        break;
    case VIEW_CIVDATA:
        DrawCivData(playerindex);
        break;
    case VIEW_CITYDETAIL:
        DrawCityDetail(playerindex);
        break;
    default:
        DrawMainView(playerindex);
        break;
    }
}

void StateGame::DrawIcons(const bool withtext, const int32_t texty, const bool centered, const int8_t maxdisplayicons)
{
    // must draw black background first because info text may have overdrawn map
    *DRAW_COLORS=PALETTE_BLACK << 4 | PALETTE_BLACK;
    rect(0,0,16,SCREEN_SIZE-16);
// TODO - make sure m_menuidx<9 and offset menu if not
    int8_t moffset=(m_menuidx<maxdisplayicons ? 0 : maxdisplayicons);
    int32_t dy=0;
    for(size_t i=moffset; i<countof(m_availableicons) && i<moffset+maxdisplayicons; i++)
    {
        if(m_availableicons[i]!=ICON_NONE)
        {
            if(i==m_menuidx)
            {
                *DRAW_COLORS=PALETTE_WHITE << 4 | PALETTE_WHITE;
                rect(0,dy,16,16);
                *DRAW_COLORS=PALETTE_BLACK << 4 | PALETTE_WHITE;
            }
            else
            {
                *DRAW_COLORS=PALETTE_WHITE << 4 | PALETTE_BLACK;
            }

            blitMasked(icongfx,icongfxalpha,0,dy,16,16,icondata[m_availableicons[i]].xidx*16,icondata[m_availableicons[i]].yidx*16,icongfxwidth,BLIT_1BPP);

            dy+=16;
        }
    }

    if(withtext==true)
    {
        TextPrinter tp;
        tp.SetCustomFont(&Font5x7::Instance());
        if(m_menuidx>=0 && m_menuidx<countof(m_availableicons) && m_availableicons[m_menuidx]!=ICON_NONE)
        {
            if(centered==false)
            {
                tp.Print(icondata[m_availableicons[m_menuidx]].name,1,texty,100,PALETTE_WHITE);
            }
            else
            {
                tp.PrintCentered(icondata[m_availableicons[m_menuidx]].name,SCREEN_SIZE/2,texty,100,PALETTE_WHITE);
            }
        }
    }

}

void StateGame::DrawMainView(const uint8_t playerindex)
{
    Unit *unitinfo[32]={nullptr};     // up to 32 unit info shown - could possibly have up to 81 units on screen at once, but we need some limit
    int32_t unitinfoidx=0;
    City *cityinfo[16]={nullptr};     // up to 16 city info shown
    int32_t cityinfoidx=0;
    Unit *selectedunitinfo=nullptr;
    TextPrinter tp;
    tp.SetCustomFont(&Font5x7::Instance());

    // draw terrain
    int32_t sx=16;
    int32_t sy=0;
    for(int y=m_mapy-4; y<m_mapy+5; y++,sy+=16)
    {
        sx=16;
        for(int x=m_mapx-4; x<m_mapx+5; x++,sx+=16)
        {
            TerrainTile t=m_map->GetTile(x,y);
            for(int i=0; i<4; i++)
            {
                SpriteSheetPos spos=t.GetSpriteSheetPos(i);
                if(spos.m_xidx>=0 && spos.m_yidx>=0)
                {
                    blitMasked(sprite,spritealpha,sx,sy,16,16,(spos.m_xidx*16),(spos.m_yidx*16),spritewidth,BLIT_2BPP);
                }
            }
        }
    }

    // draw cities
    for(size_t i=0; i<countof(m_game->GetGameData().m_city); i++)
    {
        City *c=&(m_game->GetGameData().m_city[i]);
        if(c->population>0)
        {
            // delta x,y between current pos on map
            const int32_t dx=c->x-m_mapx;
            const int32_t dy=c->y-m_mapy;

            if(dx>=-4 && dx<=4 && dy>=-4 && dy<=4)
            {
                // sprite idx
                SpriteSheetPos spos=m_game->GetCitySpriteSheetPos(i);

                // screen x,y
                sx=((dx+4)*16)+16;
                sy=(dy+4)*16;

                if(spos.m_xidx>=0 && spos.m_yidx>=0)
                {
                    // box around city - show city wall back if city has improvement
                    if((c->improvements & (0x01 << IMPROVEMENT_CITYWALLS)) == (0x01 << IMPROVEMENT_CITYWALLS))
                    {
                        blitMasked(sprite,spritealpha,sx,sy,16,16,(9*16),(5*16),spritewidth,BLIT_2BPP);
                    }
                    else
                    {
                        blitMasked(sprite,spritealpha,sx,sy,16,16,(8*16),(5*16),spritewidth,BLIT_2BPP);
                    }
                    // city sprite
                    blitMasked(sprite,spritealpha,sx,sy,16,16,(spos.m_xidx*16),(spos.m_yidx*16),spritewidth,BLIT_2BPP);
                    // city wall front (on top of city sprite)
                    if((c->improvements & (0x01 << IMPROVEMENT_CITYWALLS)) == (0x01 << IMPROVEMENT_CITYWALLS))
                    {
                        blitMasked(sprite,spritealpha,sx,sy,16,16,(10*16),(5*16),spritewidth,BLIT_2BPP);
                    }

                    OutputStringStream ostr;
                    ostr << c->population;
                    PrintInfo(ostr.Buffer(),sx+8,sy+4,2,(c->owner!=m_game->PlayerCivIndex(playerindex) ? PALETTE_BROWN : PALETTE_WHITE),PALETTE_BLACK);

                }
                if(m_showinfo && cityinfoidx<countof(cityinfo))
                {
                    cityinfo[cityinfoidx++]=c;
                }
            }
        }
    }

    // selected unit
    Unit *su=nullptr;
    if(m_selecttype==SELECT_UNIT && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_unit))
    {
        su=&(m_game->GetGameData().m_unit[m_selectidx]);
    }

    // draw units
    for(size_t i=0; i<countof(m_game->GetGameData().m_unit); i++)
    {
        // TODO - if other units are in same space as selected unit - don't show them
        // TODO - only show selected unit if in city or embarked

        Unit *u=&(m_game->GetGameData().m_unit[i]);
        // don't show unit if it's in a city or embarked (select unit in a city will show later) or its in the same space as a selected unit
        if((u->flags & UNIT_ALIVE) == UNIT_ALIVE && m_game->CityIndexAtLocation(u->x,u->y)<0 && m_game->UnitEmbarkedShipIndex(i)<0 && (!su || (su->x!=u->x || su->y!=u->y)))
        {
            // delta x,y between current pos on map
            const int32_t dx=u->x-m_mapx;
            const int32_t dy=u->y-m_mapy;

            // make sure tile is visible before drawing
            // draw if within visible map
            if(dx>=-4 && dx<=4 && dy>=-4 && dy<=4)
            {
                // sprite idx
                SpriteSheetPos spos(unitdata[u->type].xidx,unitdata[u->type].yidx);

                // screen x,y
                sx=((dx+4)*16)+16;
                sy=(dy+4)*16;

                if(spos.m_xidx>=0 && spos.m_yidx>=0)
                {
                    // if this is the selected unit - blink every .5 seconds (30 frames) (not selected unit - show always)
                    if(m_selecttype!=SELECT_UNIT || (m_selecttype==SELECT_UNIT && i!=m_selectidx) || (m_selecttype==SELECT_UNIT && m_selectidx==i && ((m_blinkticks/30)%2)==0))
                    {
                        if(u->owner!=m_game->PlayerCivIndex(playerindex))
                        {
                            *DRAW_COLORS=(PALETTE_CYAN << 4) | PALETTE_BROWN;
                        }
                        else
                        {
                            *DRAW_COLORS=(PALETTE_CYAN << 4) | PALETTE_WHITE;
                        }
                        if(m_game->UnitCountAtLocation(u->x,u->y)>1)
                        {
                            rect(sx-1,sy-1,16,16);
                        }
                        rect(sx,sy,16,16);
                        blitMasked(sprite,spritealpha,sx,sy,16,16,(spos.m_xidx*16),(spos.m_yidx*16),spritewidth,BLIT_2BPP);
                        if(u->flags & UNIT_SENTRY)
                        {
                            PrintInfo("S",sx+8,sy+4,1,PALETTE_WHITE,PALETTE_BLACK);
                        }
                    }
                    if(m_showinfo && unitinfoidx<countof(unitinfo))
                    {
                        unitinfo[unitinfoidx++]=u;
                    }
                }

            }

        }
    }

    // draw any sprite overlays (after we've drawn terrain, cities, and units, but before select unit)
    sx=16;
    sy=0;
    for(int y=m_mapy-4; y<m_mapy+5; y++,sy+=16)
    {
        sx=16;
        for(int x=m_mapx-4; x<m_mapx+5; x++,sx+=16)
        {
            MapCoord mc(m_map->Width(),m_map->Height(),x,y);
            m_game->DrawSpriteOverlay(mc.X(),mc.Y(),sx,sy);
        }
    }

    // if we have a unit select - draw it again so it will be on top of everything else
    if(su)
    {
        // selected unit info last so it's on top of everything else
        if(m_showinfo)
        {
            selectedunitinfo=su;
        }

        if(((m_blinkticks/30)%2)==0)
        {
            // delta x,y between current pos on map
            const int32_t dx=su->x-m_mapx;
            const int32_t dy=su->y-m_mapy;

            // sprite idx
            SpriteSheetPos spos(unitdata[su->type].xidx,unitdata[su->type].yidx);

            // screen x,y
            sx=((dx+4)*16)+16;
            sy=(dy+4)*16;

            if(sx>=0 && sy>=0 && sx<SCREEN_SIZE && sy<SCREEN_SIZE)
            {
                if(su->owner!=m_game->PlayerCivIndex(playerindex))
                {
                    *DRAW_COLORS=(PALETTE_CYAN << 4) | PALETTE_BROWN;
                }
                else
                {
                    *DRAW_COLORS=(PALETTE_CYAN << 4) | PALETTE_WHITE;
                }
                if(m_game->UnitCountAtLocation(su->x,su->y)>1)
                {
                    rect(sx-1,sy-1,16,16);
                }
                rect(sx,sy,16,16);
                blitMasked(sprite,spritealpha,sx,sy,16,16,(spos.m_xidx*16),(spos.m_yidx*16),spritewidth,BLIT_2BPP);
                if(su->flags & UNIT_SENTRY)
                {
                    PrintInfo("S",sx+8,sy+4,1,PALETTE_WHITE,PALETTE_BLACK);
                }
            }
        }
    }

    // need to print info after units/cities so it's on top
    if(m_showinfo)
    {
        // show unit info
        for(size_t i=0; i<unitinfoidx; i++)
        {
            Unit *u=unitinfo[i];
            // delta x,y between current pos on map
            const int32_t dx=u->x-m_mapx;
            const int32_t dy=u->y-m_mapy;
            // screen x,y
            sx=((dx+4)*16)+16;
            sy=(dy+4)*16;
            PrintInfo(civname[u->owner],sx+8,sy-16,100,PALETTE_CYAN,PALETTE_BLACK);
            PrintInfo(unitdata[u->type].name,sx+8,sy-8,100,PALETTE_WHITE,PALETTE_BLACK);
        }

        // show city info
        for(size_t i=0; i<cityinfoidx; i++)
        {
            City *c=cityinfo[i];
            // delta x,y between current pos on map
            const int32_t dx=c->x-m_mapx;
            const int32_t dy=c->y-m_mapy;
            // screen x,y
            sx=((dx+4)*16)+16;
            sy=(dy+4)*16;
            PrintInfo(civname[c->owner],sx+8,sy-16,100,PALETTE_CYAN,PALETTE_BLACK);
            // we have pointer to city but need idx, so figure out offset in city array
            PrintInfo(cityname[c-m_game->GetGameData().m_city],sx+8,sy-8,100,PALETTE_WHITE,PALETTE_BLACK);
        }

        if(selectedunitinfo)
        {
            // delta x,y between current pos on map
            const int32_t dx=selectedunitinfo->x-m_mapx;
            const int32_t dy=selectedunitinfo->y-m_mapy;
            // screen x,y
            sx=((dx+4)*16)+16;
            sy=(dy+4)*16;
            PrintInfo(civname[selectedunitinfo->owner],sx+8,sy-16,100,PALETTE_CYAN,PALETTE_BLACK);
            PrintInfo(unitdata[selectedunitinfo->type].name,sx+8,sy-8,100,PALETTE_WHITE,PALETTE_BLACK);
        }
    }

    DrawIcons(true,SCREEN_SIZE-16,false,9);

    OutputStringStream ostr;

    //blitMasked(icongfx,icongfxalpha,0,0,SCREEN_SIZE,64,0,0,icongfxwidth,BLIT_1BPP);

    /*
    TextPrinter tp;
    tp.SetCustomFont(&Font5x7::Instance());

    *DRAW_COLORS=PALETTE_WHITE;
    OutputStringStream ostr;
    ostr << m_mapx  << "," << m_mapy;

    //text(ostr.Buffer(),0,150);

    tp.Print(ostr.Buffer(),1,144,100,PALETTE_WHITE);

    const TerrainTile mt=m_map->GetTile(m_mapx,m_mapy);

    tp.PrintWrapped(TerrainTile::ClimateDesc[mt.Climate()],0,144,100,160,PALETTE_WHITE,TextPrinter::JUSTIFY_RIGHT);

    ostr.Clear();
    ostr << TerrainTile::TerrainDesc[mt.Type()];
    if(mt.Resource()!=TerrainTile::RESOURCE_NONE)
    {
        ostr << ", " << TerrainTile::ResourceDesc[mt.Resource()];
    }

    tp.Print(ostr.Buffer(),1,152,100,PALETTE_WHITE);
    */

    if(m_selecttype==SELECT_NONE)
    {
        ostr.Clear();
        const TerrainTile mt=m_map->GetTile(m_mapx,m_mapy);

        ostr << TerrainTile::TerrainDesc[mt.Type()];
        if(mt.Resource()!=TerrainTile::RESOURCE_NONE)
        {
            ostr << ", " << TerrainTile::ResourceDesc[mt.Resource()];
        }
        tp.Print(ostr.Buffer(),1,SCREEN_SIZE-8,100,PALETTE_WHITE);
    }
    // if unit selected - show info about it
    else if(m_selecttype==SELECT_UNIT && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_unit))
    {
        int32_t x=1;
        ostr.Clear();
        Unit *u=&(m_game->GetGameData().m_unit[m_selectidx]);
        //ostr << unitdata[u->type].name << " " << ((u->flags & UNIT_VETERAN) == UNIT_VETERAN ? "Veteran" : "") << u->movesleft << " Move" << (u->movesleft > 1 ? "s" : "");
        //tp.Print(ostr.Buffer(),1,SCREEN_SIZE-8,100,PALETTE_WHITE);
        x+=tp.Print(unitdata[u->type].name,x,SCREEN_SIZE-8,100,PALETTE_CYAN);
        if((u->flags & UNIT_VETERAN) == UNIT_VETERAN)
        {
            x+=tp.Print(" Vet",x,SCREEN_SIZE-8,100,PALETTE_BROWN);
        }
        *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
        x+=2;
        blitMasked(icongfx,icongfxalpha,x,SCREEN_SIZE-8,8,8,16,8,icongfxwidth,BLIT_1BPP);
        x+=9;
        ostr << u->movesleft;
        x+=tp.Print(ostr.Buffer(),x,SCREEN_SIZE-8,100,PALETTE_WHITE);

        *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
        x+=2;
        blitMasked(icongfx,icongfxalpha,x,SCREEN_SIZE-8,8,8,16,0,icongfxwidth,BLIT_1BPP);
        x+=9;
        ostr.Clear();
        const uint32_t home=m_selectidx/UNITS_PER_CITY;
        ostr << cityname[home][0] << cityname[home][1] << cityname[home][2];
        x+=tp.Print(ostr.Buffer(),x,SCREEN_SIZE-8,5,PALETTE_CYAN);
        // TODO - print attack/defense? (action - sentry, etc)
        if((u->flags & UNIT_SENTRY))
        {
            x+=tp.Print(" Sentry",x,SCREEN_SIZE-8,7,PALETTE_WHITE);
        }
    }
    // if city selected - show info about it
    else if(m_selecttype==SELECT_CITY && m_selectidx>=0 && m_selectidx<countof(m_game->GetGameData().m_city))
    {
        int32_t x=1;
        ostr.Clear();
        City *c=&(m_game->GetGameData().m_city[m_selectidx]);
        x+=tp.Print(cityname[m_selectidx],x,SCREEN_SIZE-8,100,PALETTE_CYAN);
        ostr << " " << c->population;
        x+=tp.Print(ostr.Buffer(),x,SCREEN_SIZE-8,100,PALETTE_BROWN);
        const int32_t uc=m_game->UnitCountAtLocation(m_mapx,m_mapy);
        if(uc)
        {
            ostr.Clear();
            ostr << " " << uc << " In City";
            x+=tp.Print(ostr.Buffer(),x,SCREEN_SIZE-8,100,PALETTE_WHITE);
        }
    }

    DrawHourGlass(playerindex);
}

void StateGame::DrawMap(const uint8_t playerindex)
{
    for(int32_t y=0; y<m_map->Height(); y++)
    {
        for(int32_t x=0; x<m_map->Width(); x++)
        {
            BaseTerrain::TerrainType bt=m_map->GetBaseType(x,y);

            if(bt==BaseTerrain::BASETERRAIN_WATER)
            {
                *DRAW_COLORS=PALETTE_CYAN;
            }
            else
            {
                *DRAW_COLORS=PALETTE_BROWN;
            }
            line(x+16,y+32,x+16,y+32);
        }
    }

    // draw outline of were main view is
    MapCoord mc(m_map->Width(),m_map->Height(),0,0);
    *DRAW_COLORS=PALETTE_BLACK;
    for(int32_t dy=m_mapy-4; dy<m_mapy+5; dy+=8)
    {
        for(int32_t dx=m_mapx-4; dx<m_mapx+5; dx++)
        {
            mc.Set(dx,dy);
            if(mc.X()>=0 && mc.X()<m_map->Width() && mc.Y()>=0 && mc.Y()<m_map->Height())
            {
                line(mc.X()+16,mc.Y()+32,mc.X()+16,mc.Y()+32);
            }
        }
    }
    for(int32_t dx=m_mapx-4; dx<m_mapx+5; dx+=8)
    {
        for(int32_t dy=m_mapy-4; dy<m_mapy+5; dy++)
        {
            mc.Set(dx,dy);
            if(mc.X()>=0 && mc.X()<m_map->Width() && mc.Y()>=0 && mc.Y()<m_map->Height())
            {
                line(mc.X()+16,mc.Y()+32,mc.X()+16,mc.Y()+32);
            }
        }
    }

    // draw cities (white dot with black surround)
    for(size_t i=0; i<countof(m_game->GetGameData().m_city); i++)
    {
        if(m_game->GetGameData().m_city[i].population>0)
        {
            mc.Set(m_game->GetGameData().m_city[i].x,m_game->GetGameData().m_city[i].y);
            for(int32_t y=mc.Y()-1; y<mc.Y()+2; y++)
            {
                for(int32_t x=mc.X()-1; x<mc.X()+2; x++)
                {
                    MapCoord c(m_map->Width(),m_map->Height(),x,y);
                    if(mc.X()!=c.X() || mc.Y()!=c.Y())
                    {
                        *DRAW_COLORS=PALETTE_BLACK;
                    }
                    else
                    {
                        // brown for enemy cities - always white for our own
                        if(m_game->GetGameData().m_city[i].owner==m_game->PlayerCivIndex(playerindex))
                        {
                            *DRAW_COLORS=PALETTE_WHITE;
                        }
                        else
                        {
                            *DRAW_COLORS=PALETTE_BROWN;
                        }
                    }
                    line(c.X()+16,c.Y()+32,c.X()+16,c.Y()+32);
                }
            }
        }
    }

    // draw units (black dot)
    *DRAW_COLORS=PALETTE_BLACK;
    for(size_t i=0; i<countof(m_game->GetGameData().m_unit); i++)
    {
        if((m_game->GetGameData().m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE)
        {
            mc.Set(m_game->GetGameData().m_unit[i].x,m_game->GetGameData().m_unit[i].y);
            line(mc.X()+16,mc.Y()+32,mc.X()+16,mc.Y()+32);
        }
    }

    DrawIcons(true,SCREEN_SIZE-16,true,9);

    DrawHourGlass(playerindex);

}

void StateGame::DrawCivData(const uint8_t playerindex)
{

    TextPrinter tp;
    tp.SetCustomFont(&Font5x7::Instance());
    OutputStringStream ostr;

    tp.PrintCentered(civname[m_game->PlayerCivIndex(playerindex)],SCREEN_SIZE/2,1,100,PALETTE_CYAN);

    int32_t x=18;
    int32_t y=16;
    x+=tp.Print("Turn ",x,y,5,PALETTE_CYAN);
    ostr << m_game->GetGameData().m_gameturn;
    x+=tp.Print(ostr.Buffer(),x,y,10,PALETTE_BROWN);

    tp.Print("Coffers",74,y,7,PALETTE_CYAN);

    x=112;
    *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
    blitMasked(icongfx,icongfxalpha,x,y,8,8,0,8,icongfxwidth,BLIT_1BPP);
    x+=9;
    ostr.Clear();
    ostr << m_game->GetGameData().m_civ[m_game->PlayerCivIndex(playerindex)].gold;
    tp.Print(ostr.Buffer(),x,y,10,PALETTE_BROWN);

    int32_t prodres=0;
    int32_t prodfood=0;
    int32_t prodgold=0;
    int32_t upres=0;
    int32_t upfood=0;
    int32_t upgold=0;
    for(size_t i=0; i<countof(m_game->GetGameData().m_city); i++)
    {
        if(m_game->GetGameData().m_city[i].population>0 && m_game->GetGameData().m_city[i].owner==m_game->PlayerCivIndex(playerindex))
        {
            CityProduction cprod=m_game->GetCityProduction(i);
            prodres+=cprod.totalresources;
            prodfood+=cprod.totalfood;
            prodgold+=cprod.totalgold;

            upres+=cprod.totalupkeepresources;
            upfood+=cprod.totalupkeepfood;
            upgold+=cprod.totalupkeepgold;
        }
    }
    /* - city upkeep already has unit upkeep
    // add unit upkeep
    for(size_t i=0; i<countof(m_game->GetGameData().m_unit); i++)
    {
        if((m_game->GetGameData().m_unit[i].flags & UNIT_ALIVE)==UNIT_ALIVE && m_game->GetGameData().m_unit[i].owner==m_game->PlayerCivIndex(playerindex))
        {
            upres+=unitdata[m_game->GetGameData().m_unit[i].type].consumeresources;
            upfood+=unitdata[m_game->GetGameData().m_unit[i].type].consumefood;
            upgold+=unitdata[m_game->GetGameData().m_unit[i].type].consumegold;
        }
    }
    */

    y=26;
    x=18;
    x+=tp.Print("Production",x,y,10,PALETTE_CYAN);
    y+=8;

    *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
    blitMasked(icongfx,icongfxalpha,16,y,8,8,0,0,icongfxwidth,BLIT_1BPP);
    blitMasked(icongfx,icongfxalpha,64,y,8,8,8,0,icongfxwidth,BLIT_1BPP);
    blitMasked(icongfx,icongfxalpha,112,y,8,8,0,8,icongfxwidth,BLIT_1BPP);

    ostr.Clear();
    ostr << prodfood;
    tp.Print(ostr.Buffer(),16+9,y,10,PALETTE_BROWN);
    
    ostr.Clear();
    ostr << prodres;
    tp.Print(ostr.Buffer(),64+9,y,10,PALETTE_BROWN);
    
    ostr.Clear();
    ostr << prodgold;
    tp.Print(ostr.Buffer(),112+9,y,10,PALETTE_BROWN);

    y=44;
    x=18;
    x+=tp.Print("Upkeep",x,y,10,PALETTE_CYAN);
    y+=8;

    *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
    blitMasked(icongfx,icongfxalpha,16,y,8,8,0,0,icongfxwidth,BLIT_1BPP);
    blitMasked(icongfx,icongfxalpha,64,y,8,8,8,0,icongfxwidth,BLIT_1BPP);
    blitMasked(icongfx,icongfxalpha,112,y,8,8,0,8,icongfxwidth,BLIT_1BPP);

    ostr.Clear();
    ostr << upfood;
    tp.Print(ostr.Buffer(),16+9,y,10,PALETTE_BROWN);
    
    ostr.Clear();
    ostr << upres;
    tp.Print(ostr.Buffer(),64+9,y,10,PALETTE_BROWN);
    
    ostr.Clear();
    ostr << upgold;
    tp.Print(ostr.Buffer(),112+9,y,10,PALETTE_BROWN);

    DrawIcons(true,SCREEN_SIZE-8,true,1);    // do this first because it clears the left column of the screen

    *DRAW_COLORS=PALETTE_WHITE;
    line(0,62,SCREEN_SIZE,62);

    // Tax Rate
    // Income
    // Upkeep

    int32_t citycnt=0;
    int8_t shown=0;
    y=64;
    for(size_t i=0; i<countof(m_game->GetGameData().m_city); i++)
    {
        if(m_game->GetGameData().m_city[i].population>0 && m_game->GetGameData().m_city[i].owner==m_game->PlayerCivIndex(playerindex))
        {
            if(m_submenuidx<=citycnt && shown<5)
            {
                x=0;
                const SpriteSheetPos ss=m_game->GetCitySpriteSheetPos(i);
                blitMasked(sprite,spritealpha,x,y,16,16,ss.m_xidx*16,ss.m_yidx*16,spritewidth,BLIT_2BPP);
                x+=18;
                x+=tp.Print(cityname[i],x,y,10,PALETTE_CYAN);

                ostr.Clear();
                ostr << " (" << m_game->GetGameData().m_city[i].population << ") ";
                x+=tp.Print(ostr.Buffer(),x,y,10,PALETTE_WHITE);

                ostr.Clear();
                const uint8_t producing=m_game->GetGameData().m_city[i].producing;
                if(buildingxref[producing].buildingtype==BUILDINGTYPE_UNIT)
                {
                    ostr << unitdata[buildingxref[producing].building].name;
                }
                else if(buildingxref[producing].buildingtype==BUILDINGTYPE_IMPROVEMENT)
                {
                    ostr << improvementdata[buildingxref[producing].building].name;
                }
                else
                {
                    ostr << "-";
                }
                x+=tp.Print(ostr.Buffer(),x,y,16,PALETTE_BROWN);

                const CityProduction cprod=m_game->GetCityProduction(i);
                y+=8;
                *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
                blitMasked(icongfx,icongfxalpha,16,y,8,8,0,0,icongfxwidth,BLIT_1BPP);
                blitMasked(icongfx,icongfxalpha,64,y,8,8,8,0,icongfxwidth,BLIT_1BPP);
                blitMasked(icongfx,icongfxalpha,112,y,8,8,0,8,icongfxwidth,BLIT_1BPP);

                ostr.Clear();
                ostr << cprod.totalfood;
                tp.Print(ostr.Buffer(),16+9,y,10,PALETTE_BROWN);
                ostr.Clear();
                ostr << cprod.totalresources;
                tp.Print(ostr.Buffer(),64+9,y,10,PALETTE_BROWN);
                ostr.Clear();
                ostr << cprod.totalgold;
                tp.Print(ostr.Buffer(),112+9,y,10,PALETTE_BROWN);

                shown++;
                y+=8;
            }
            citycnt++;
        }
    }
    m_selectidx=citycnt;   // store the city count in the select index so we know how much we can scroll up/down in input handler

    DrawHourGlass(playerindex);

}

void StateGame::DrawCityDetail(const uint8_t playerindex)
{
    int32_t xpos=0;
    TextPrinter tp;
    tp.SetCustomFont(&Font5x7::Instance());
    OutputStringStream ostr;

    City *c=&(m_game->GetGameData().m_city[m_selectidx]);

    // city might have been destoryed or captured while we're in this screen, so check and set back to view none
    if(c->population==0 || c->owner!=m_game->PlayerCivIndex(playerindex))
    {
        m_view=VIEW_NONE;
        m_selectidx=-1;
        return;
    }

    tp.PrintCentered(cityname[m_selectidx],56,0,100,PALETTE_CYAN);
    ostr << "Pop " << c->population;
    tp.Print(ostr.Buffer(),96,0,10,PALETTE_WHITE);

    DrawIcons(true,SCREEN_SIZE-8,true,5);

    MapCoord mc(m_map->Width(),m_map->Height(),0,0);
    for(int32_t dy=-2; dy<3; dy++)
    {
        for(int32_t dx=-2; dx<3; dx++)
        {
            mc.Set(m_mapx+dx,m_mapy+dy);
            TerrainTile tt=m_map->GetTile(mc.X(),mc.Y());
            for(int i=0; i<6; i++)
            {
                SpriteSheetPos ss=tt.GetSpriteSheetPos(i);
                if(ss.m_xidx>=0 && ss.m_yidx>=0)
                {
                    blitMasked(sprite,spritealpha,16+((dx+2)*16),8+((dy+2)*16),16,16,ss.m_xidx*16,ss.m_yidx*16,spritewidth,BLIT_2BPP);
                }
            }
            if(dx==0 && dy==0)
            {
                // TODO - draw city sprite with population
                SpriteSheetPos ss=m_game->GetCitySpriteSheetPos(m_selectidx);
                if(ss.m_xidx>=0 && ss.m_yidx>=0)
                {
                    // box around city - show city wall back if city has improvement
                    if((c->improvements & (0x01 << IMPROVEMENT_CITYWALLS)) == (0x01 << IMPROVEMENT_CITYWALLS))
                    {
                        blitMasked(sprite,spritealpha,16+((dx+2)*16),8+((dy+2)*16),16,16,(9*16),(5*16),spritewidth,BLIT_2BPP);
                    }
                    else
                    {
                        blitMasked(sprite,spritealpha,16+((dx+2)*16),8+((dy+2)*16),16,16,(8*16),(5*16),spritewidth,BLIT_2BPP);
                    }
                    // city sprite
                    blitMasked(sprite,spritealpha,16+((dx+2)*16),8+((dy+2)*16),16,16,ss.m_xidx*16,ss.m_yidx*16,spritewidth,BLIT_2BPP);
                    // city wall front (on top of city sprite)
                    if((c->improvements & (0x01 << IMPROVEMENT_CITYWALLS)) == (0x01 << IMPROVEMENT_CITYWALLS))
                    {
                        blitMasked(sprite,spritealpha,16+((dx+2)*16),8+((dy+2)*16),16,16,(10*16),(5*16),spritewidth,BLIT_2BPP);
                    }
                }
            }
        }
    }

    tp.Print("Improvements",96,8,12,PALETTE_CYAN);
    int32_t sy=16;
    for(size_t i=0; i<16; i++)
    {
        if((c->improvements & (0x01 << i)) == (0x01 << i))
        {
            // print improvement name
            tp.Print(improvementdata[i].name,96,sy,20,(m_submenuidx2==i ? PALETTE_WHITE : PALETTE_BROWN));
            // show upkeep gold if submenu is 1
            if(m_submenuidx==1 && m_submenuidx2<0)
            {
                int32_t x=(SCREEN_SIZE-8)-((improvementdata[i].upkeepgold-1)*3);
                for(int32_t g=0; g<improvementdata[i].upkeepgold; g++)
                {
                    *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
                    blitMasked(icongfx,icongfxalpha,x,sy,8,8,0,8,icongfxwidth,BLIT_1BPP);
                    x+=3;
                }
            }
            if(m_submenuidx==1 && m_submenuidx2==i)
            {
                ostr.Clear();
                ostr << improvementdata[i].sellgold;
                int32_t w=tp.Print(ostr.Buffer(),SCREEN_SIZE+1,0,10,PALETTE_WHITE);     // "print" off the screen just to get the length
                tp.PrintWrapped(ostr.Buffer(),80,sy,10,80,PALETTE_CYAN,TextPrinter::JUSTIFY_RIGHT);
                *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
                blitMasked(icongfx,icongfxalpha,SCREEN_SIZE-(w+8),sy,8,8,0,8,icongfxwidth,BLIT_1BPP);
            }
            sy+=8;
        }
    }

    CityProduction prod=m_game->GetCityProduction(m_selectidx);

    tp.Print("Production",1,88,10,PALETTE_CYAN);

    *DRAW_COLORS=PALETTE_WHITE << 4 || PALETTE_BLACK;
    blitMasked(icongfx,icongfxalpha,0,96,8,8,0,0,icongfxwidth,BLIT_1BPP);       // food
    blitMasked(icongfx,icongfxalpha,30,96,8,8,8,0,icongfxwidth,BLIT_1BPP);      // resources
    blitMasked(icongfx,icongfxalpha,60,96,8,8,0,8,icongfxwidth,BLIT_1BPP);      // gold

    ostr.Clear();
    ostr << prod.totalfood;
    tp.Print(ostr.Buffer(),9,96,10,PALETTE_BROWN);

    ostr.Clear();
    ostr << prod.totalresources;
    tp.Print(ostr.Buffer(),39,96,10,PALETTE_BROWN);

    ostr.Clear();
    ostr << prod.totalgold;
    tp.Print(ostr.Buffer(),69,96,10,PALETTE_BROWN);

    ostr.Clear();
    xpos=1;
    xpos+=tp.Print("Building ",xpos,104,10,PALETTE_CYAN);
    *DRAW_COLORS=PALETTE_WHITE << 4 || PALETTE_BLACK;
    blitMasked(icongfx,icongfxalpha,xpos,104,8,8,8,0,icongfxwidth,BLIT_1BPP);
    xpos+=9;
    ostr << c->shields << " / ";

    const char *buildname=nullptr;
    int32_t buildresources=0;
    int32_t buildcost=0;
    // progress of building
    // TODO - build cost should be % of total cost based on % of resouces already there
    if(buildingxref[c->producing].buildingtype==BUILDINGTYPE_UNIT)
    {
        buildname=unitdata[buildingxref[c->producing].building].name;
        buildresources=unitdata[buildingxref[c->producing].building].buildresources;
        buildcost=unitdata[buildingxref[c->producing].building].buildgold;
    }
    else if(buildingxref[c->producing].buildingtype==BUILDINGTYPE_IMPROVEMENT)
    {
        buildname=improvementdata[buildingxref[c->producing].building].name;
        buildresources=improvementdata[buildingxref[c->producing].building].buildresources;
        buildcost=improvementdata[buildingxref[c->producing].building].buildgold;
    }
    else
    {
        buildname="None";
    }
    ostr << buildresources;
    tp.Print(ostr.Buffer(),xpos,104,16,PALETTE_BROWN);
    xpos=1;
    ostr.Clear();
    ostr << buildname;
    xpos+=tp.Print(ostr.Buffer(),xpos,112,32,PALETTE_BROWN);
    ostr.Clear();
    if(m_submenuidx==0 || buildingxref[c->producing].buildingtype!=BUILDINGTYPE_UNIT)
    {
        xpos+=tp.Print(" (",xpos,112,32,PALETTE_BROWN);
        *DRAW_COLORS=PALETTE_WHITE << 4 || PALETTE_BLACK;
        blitMasked(icongfx,icongfxalpha,xpos,112,8,8,0,8,icongfxwidth,BLIT_1BPP);
        xpos+=9;
        ostr << buildcost << ")"; // print cost of building
        tp.Print(ostr.Buffer(),xpos,112,32,PALETTE_BROWN);
    }
    else
    {
        ostr << " ADM " << unitdata[buildingxref[c->producing].building].attack << "/" << unitdata[buildingxref[c->producing].building].defense << "/" << unitdata[buildingxref[c->producing].building].moves;
        tp.Print(ostr.Buffer(),xpos,112,32,PALETTE_WHITE);
    }

    ostr.Clear();
    tp.Print("Storage",1,120,10,PALETTE_CYAN);
    *DRAW_COLORS=PALETTE_WHITE << 4 || PALETTE_BLACK;
    blitMasked(icongfx,icongfxalpha,0,128,8,8,0,0,icongfxwidth,BLIT_1BPP);
    ostr << c->food << " / " << m_game->CityFoodStorage(m_selectidx);      // calculate total storage space available (granary etc)
    tp.Print(ostr.Buffer(),9,128,11,PALETTE_BROWN);

    ostr.Clear();
    tp.Print("Upkeep",1,136,10,PALETTE_CYAN);
    *DRAW_COLORS=PALETTE_WHITE << 4 || PALETTE_BLACK;
    blitMasked(icongfx,icongfxalpha,0,144,8,8,0,8,icongfxwidth,BLIT_1BPP);
    ostr << prod.totalupkeepgold;
    tp.Print(ostr.Buffer(),9,144,5,PALETTE_BROWN);

    *DRAW_COLORS=PALETTE_WHITE << 4 || PALETTE_BLACK;
    int32_t idx=0;
    for(int32_t dy=-2; dy<3; dy++)
    {
        for(int32_t dx=-2; dx<3; dx++,idx++)
        {
            int32_t sx=16+((dx+2)*16);
            int32_t sy=8+((dy+2)*16);
            for(int32_t i=0; i<prod.tile[idx].food && i<6; i++,sx+=4)
            {
                if(i>0 && (i%3)==0)
                {
                    sx-=12;
                    sy+=2;
                }
                blitMasked(icongfx,icongfxalpha,sx,sy,8,8,0,0,icongfxwidth,BLIT_1BPP);
            }
            sx=16+((dx+2)*16);
            sy=14+((dy+2)*16);
            for(int32_t i=0; i<prod.tile[idx].resources && i<6; i++,sx+=4)
            {
                if(i>0 && (i%3)==0)
                {
                    sx-=12;
                    sy+=2;
                }
                blitMasked(icongfx,icongfxalpha,sx,sy,8,8,8,0,icongfxwidth,BLIT_1BPP);
            }
        }
    }

    // units
    xpos=70;
    tp.Print("Units",xpos+4,SCREEN_SIZE-32,10,PALETTE_CYAN);
    for(size_t i=(m_selectidx*UNITS_PER_CITY); i<(m_selectidx*UNITS_PER_CITY)+UNITS_PER_CITY; i++)
    {
        Unit *u=&(m_game->GetGameData().m_unit[i]);
        if((u->flags & UNIT_ALIVE)==UNIT_ALIVE)
        {
            blitMasked(sprite,spritealpha,xpos+2,SCREEN_SIZE-24,16,16,unitdata[u->type].xidx*16,unitdata[u->type].yidx*16,spritewidth,BLIT_2BPP);
            // display upkeep if submenu is 1
            if(m_submenuidx==1)
            {
                *DRAW_COLORS=PALETTE_WHITE << 4 || PALETTE_BLACK;
                for(size_t i=0; i<unitdata[u->type].consumefood; i++)
                {
                    blitMasked(icongfx,icongfxalpha,xpos+(i*4),(SCREEN_SIZE-24)+0,8,8,0,0,icongfxwidth,BLIT_1BPP);
                }
                for(size_t i=0; i<unitdata[u->type].consumeresources; i++)
                {
                    blitMasked(icongfx,icongfxalpha,xpos+(i*4),(SCREEN_SIZE-24)+4,8,8,8,0,icongfxwidth,BLIT_1BPP);
                }
                for(size_t i=0; i<unitdata[u->type].consumegold; i++)
                {
                    blitMasked(icongfx,icongfxalpha,xpos+(i*4),(SCREEN_SIZE-24)+8,8,8,0,8,icongfxwidth,BLIT_1BPP);
                }
            }

            xpos+=18;
        }
    }

}

void StateGame::PrintInfo(const char *text, const int32_t cx, const int32_t y, const int32_t len, const int32_t fg, const int32_t bg)
{
    TextPrinter tp;
    tp.SetCustomFont(&Font5x7::Instance());
    if(bg)
    {
        for(int32_t sy=y-1; sy<y+2; sy++)
        {
            for(int32_t sx=cx-1; sx<cx+2; sx++)
            {
                if(sx!=cx || sy!=y)
                {
                    tp.PrintCentered(text,sx,sy,len,bg);
                }
            }
        }
    }
    tp.PrintCentered(text,cx,y,len,fg);
}

void StateGame::DrawHourGlass(const uint8_t playerindex)
{
    // if player turn, then print on screen
    if(m_game->IsPlayerTurn(playerindex) && m_game->CivilizationAlive(m_game->PlayerCivIndex(playerindex))==true)
    {
        TextPrinter tp;
        tp.SetCustomFont(&Font5x7::Instance());
        *DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
        if(((m_game->GetGameData().m_ticks/60)%2)==0)
        {
            blitMasked(icongfx,icongfxalpha,SCREEN_SIZE-16,SCREEN_SIZE-16,16,16,8*16,1*16,icongfxwidth,BLIT_1BPP);
        }
        else
        {
            blitMasked(icongfx,icongfxalpha,SCREEN_SIZE-16,SCREEN_SIZE-16,16,16,8*16,1*16,icongfxwidth,BLIT_1BPP|BLIT_ROTATE);
        }

        OutputStringStream ostr;
        //const int64_t s=(m_game->GetGameData().m_ticks-m_game->GetGameData().m_turnstarttick)/60;
        const int64_t s=m_game->TurnTicksLeft()/60;
        if(s<60)
        {
            ostr << s << "s";
        }
        else if(s<3600)
        {
            ostr << (s/60) << "m";
        }
        else
        {
            ostr << (s/3600) << "h";
        }

        tp.PrintWrapped(ostr.Buffer(),SCREEN_SIZE-48,SCREEN_SIZE-12,10,32,PALETTE_BROWN,TextPrinter::JUSTIFY_RIGHT);
    }
}
