#pragma once

#include "simplexnoise.h"
#include "input.h"
#include "game.h"
#include "pathfinder.h"

#define DIR_NONE            0b00000000
#define DIR_NORTHWEST       0b00000001
#define DIR_NORTH           0b00000010
#define DIR_NORTHEAST       0b00000100
#define DIR_WEST            0b00001000
#define DIR_EAST            0b00010000
#define DIR_SOUTHWEST       0b00100000
#define DIR_SOUTH           0b01000000
#define DIR_SOUTHEAST       0b10000000

#define DIR_CARDINALS       DIR_NORTH | DIR_EAST | DIR_SOUTH | DIR_WEST
#define DIR_DIAGONALS       DIR_NORTHWEST | DIR_NORTHEAST | DIR_SOUTHEAST | DIR_SOUTHWEST

#define DIR_NORTHWEST_DX    -1
#define DIR_NORTH_DX         0
#define DIR_NORTHEAST_DX     1
#define DIR_WEST_DX         -1
#define DIR_EAST_DX          1
#define DIR_SOUTHWEST_DX    -1
#define DIR_SOUTH_DX         0
#define DIR_SOUTHEAST_DX     1

#define DIR_NORTHWEST_DY     -1
#define DIR_NORTH_DY         -1
#define DIR_NORTHEAST_DY     -1
#define DIR_WEST_DY           0
#define DIR_EAST_DY           0
#define DIR_SOUTHWEST_DY      1
#define DIR_SOUTH_DY          1
#define DIR_SOUTHEAST_DY      1

namespace global
{
    extern int64_t ticks;
    extern SimplexNoise *noise;
    extern Input *input;
    extern Game *game;

    void SetupGlobals();
}
