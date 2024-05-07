#pragma once

#include "iplayerstate.h"
#include "game.h"
#include "map.h"

class StateGameParams:public IStateChangeParams
{
public:
    StateGameParams(Game *game);
    ~StateGameParams();

    Game *m_game;
};

class StateGame:public IPlayerState
{
public:
    StateGame(Map *map);
    ~StateGame();

    uint8_t State() const;

    void StateChanged(const uint8_t playerindex, const uint8_t prevstate, const IStateChangeParams *params);
    bool HandleInput(const Input *input, const uint8_t playerindex);
    void Update(const int ticks, const uint8_t playerindex, Game *game);
    void Draw(const uint8_t playerindex);

    enum SelectType
    {
        SELECT_NONE=0,
        SELECT_UNIT,
        SELECT_CITY,
        SELECT_FIXEDLOC
    };

    enum ViewType
    {
        VIEW_NONE=0,
        VIEW_MAP=1,
        VIEW_UNITDETAIL=2,
        VIEW_CITYDETAIL=3,
        VIEW_CIVCITIES=4,
        VIEW_CIVDATA=5
    };

private:
    
    Game *m_game;
    Map *m_map;
    bool m_showinfo;
    int32_t m_mapx;
    int32_t m_mapy;
    int8_t m_scrollticks;
    int32_t m_blinkticks;
    int8_t m_view;
    int8_t m_menuidx;
    int8_t m_submenuidx;
    int8_t m_submenuidx2;
    int8_t m_selecttype;
    int32_t m_selectidx;        // 32 bit in case 8 bit isn't large enough for index values
    int32_t m_lastunitidx;
    int8_t m_availableicons[12];

    void DrawIcons(const bool withtext, const int32_t texty, const bool centered, const int8_t maxdisplayicons);
    void DrawMainView(const uint8_t playerindex);
    void DrawMap(const uint8_t playerindex);
    void DrawCivData(const uint8_t playerindex);
    void DrawCityDetail(const uint8_t playerindex);
    void DrawHourGlass(const uint8_t playerindex, const bool small=false);

    void PrintInfo(const char *text, const int32_t cx, const int32_t y, const int32_t len, const int32_t fg, const int32_t bg);

};
