#pragma once

#include <stdint.h>
#include <stddef.h>

#include "map.h"
#include "pathfinder.h"
#include "defines.h"

struct Civilization
{
    uint16_t gold;
    uint16_t science;
    uint8_t flags;
    uint8_t taxrate;
    uint8_t researching;
};

struct City
{
    uint16_t improvements;      // bitmask
    uint16_t shields;
    uint16_t food;
    uint8_t owner:2;
    uint8_t population:6;
    uint8_t x;
    uint8_t y;
    uint8_t producing;          // index into buildingxref
};

struct TileProduction
{
    int8_t food;
    int8_t resources;
};

struct CityProduction
{
    TileProduction tile[25];    // 5x5 matrix around city
    int32_t totalfood;
    int32_t totalresources;
    int32_t totalgold;
    int32_t unitupkeepfood;
    int32_t unitupkeepresources;
    int32_t unitupkeepgold;
    int32_t totalupkeepfood;
    int32_t totalupkeepresources;
    int32_t totalupkeepgold;
};

enum UnitFlag
{
    UNIT_ALIVE=     0b00000001,
    UNIT_VETERAN=   0b00000010,
    UNIT_SENTRY=    0b00000100,
    UNIT_AWAY=      0b00001000,
    UNIT_ROGUE=     0b00010000,
    UNIT_MOVESLEFT= 0b11100000
};

// land units (of same civ) can occupy same space, sea units cannot
// this way we can see if a land unit is "on" water then they must be embarked on the ship at the same location

struct Unit
{
    uint8_t movesleft:3;
    uint8_t flags:5;
    uint8_t owner:2;
    uint8_t type:6;   // 2 bits owner (civ 0-3) - 6 bits type
    uint8_t x;
    uint8_t y;
};

class GameData
{
public:
    GameData();
    ~GameData();

    void SetupNewGame(const uint64_t seed);
    void SaveGame();
    bool LoadGame();

    int8_t GetCivIndexFromPlayerNum(const uint8_t playernum) const;     // return -1 if player is not assigned a civ
    int8_t GetNextFreeCivIndex(const int8_t startnum, const int8_t dir) const;
    void AssignPlayerNumCivIndex(const uint8_t playernum, const int8_t civindex);
    void ClearPlayerNumCivIndex(const uint8_t playernum);
    
    bool AllPlayersReady() const;   // returns true if all connected players are ready to start

    Map *m_map;
    Pathfinder *m_pathfinder;
    uint64_t m_seed;
    uint64_t m_ticks;
    bool m_gamestarted;
    uint8_t m_civplayernum[MAX_CIVILIZATIONS];      // integer for player number of each civilization (0=CPU, 1-4 = multiplayer player)
    int64_t m_playerlastactivity[4];                // last tick count when player pressed any gamepad button (use to time out multiplayers who leave)
    bool m_playeractive[4];
    bool m_playerready[4];                          // player ready to play game

    uint32_t m_gameturn;
    uint8_t m_currentcivturn;
    uint64_t m_turnstarttick;
    uint32_t m_turntimelimit;

    Civilization m_civ[MAX_CIVILIZATIONS];
    City m_city[MAX_CITIES];
    Unit m_unit[MAX_UNITS];

private:

};
