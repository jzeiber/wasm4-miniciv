#pragma once

#include "simplexnoise.h"
#include "input.h"
#include "game.h"

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

namespace global
{
    extern int64_t ticks;
    extern SimplexNoise *noise;
    extern Input *input;
    extern Game *game;

    void SetupGlobals();
}