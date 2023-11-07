#include "unitdata.h"

const UnitData unitdata[]={
//unittype,             name,       flags,                              xidx,   yidx,   moves,  transport,  attack, defense,    cfood,  cres,   cgold,  bres,   bgold
{/*UNITTYPE_NONE,*/     nullptr,    0,                                  0,      0,      0,      0,          0,      0,          0,      0,      0,      0,      0},
{/*UNITTYPE_SETTLER,*/  "Settler",  UNITDATA_MOVE_LAND,                 8,      6,      1,      0,          0,      1,          0,      0,      0,      100,    150},
{/*UNITTYPE_MILITIA,*/  "Militia",  UNITDATA_MOVE_LAND,                 9,      6,      1,      0,          1,      1,          1,      1,      0,      50,     100},
{/*UNITTYPE_ARCHER,*/   "Archer",   UNITDATA_MOVE_LAND,                 10,     6,      1,      0,          2,      0,          1,      1,      1,      150,    200},
{/*UNITTYPE_TRIREME,*/  "Trireme",  UNITDATA_MOVE_WATER,                11,     6,      2,      1,          2,      1,          1,      1,      1,      250,    350},
{/*UNITTYPE_HORSEMAN,*/ "Horseman", UNITDATA_MOVE_LAND,                 8,      7,      3,      0,          2,      2,          1,      1,      1,      350,    400},
{/*UNITTYPE_KNIGHT,*/   "Knight",   UNITDATA_MOVE_LAND,                 9,      7,      4,      0,          4,      2,          1,      1,      2,      500,    600},
{/*UNITTYPE_SAIL,*/     "Sail",     UNITDATA_MOVE_WATER,                10,     7,      4,      2,          3,      2,          2,      2,      3,      600,    700}
};
