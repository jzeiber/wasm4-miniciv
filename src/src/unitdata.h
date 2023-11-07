#pragma once

#include <stdint.h>

enum UnitType
{
    UNITTYPE_NONE=          0,
    UNITTYPE_SETTLER=       1,
    UNITTYPE_MILITIA=       2,
    UNITTYPE_ARCHER=        3,
    UNITTYPE_TRIREME=       4,
    UNITTYPE_HORSEMAN=      5,
    UNITTYPE_KNIGHT=        6,
    UNITTYPE_SAIL=          7,
    UNITTYPE_MAX
};

enum UniDataFlag
{
    UNITDATA_MOVE_LAND=     0b00000001,
    UNITDATA_MOVE_WATER=    0b00000010,
    UNITDATA_MOVE_AIR=      0b00000100
};

struct UnitData
{
    const char *name;
    int8_t flags;
    int8_t xidx;
    int8_t yidx;
    int8_t moves;
    uint8_t transport;
    uint8_t attack;
    uint8_t defense;
    uint8_t consumefood;            // food consumption per turn
    uint8_t consumeresources;       // resource consumption per turn
    uint8_t consumegold;            // gold consumption per turn
    uint32_t buildresources;        // how many resources it takes to build
    uint32_t buildgold;             // how much gold it takes to buy
};

extern const UnitData unitdata[];