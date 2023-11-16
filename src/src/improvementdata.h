#pragma once

#include "unitdata.h"

enum Improvement
{
IMPROVEMENT_NONE=       0,
IMPROVEMENT_GRANARY=    1,
IMPROVEMENT_BARRACKS=   2,
IMPROVEMENT_CITYWALLS=  3,
IMPROVEMENT_MARKET=     4,
IMPROVEMENT_BANK=       5,
IMPROVEMENT_FACTORY=    6,
IMPROVEMENT_AQUEDUCT=   7,
IMPROVEMENT_MAX
};

enum BuildingType
{
BUILDINGTYPE_NONE=0,
BUILDINGTYPE_UNIT=1,
BUILDINGTYPE_IMPROVEMENT=2
};

enum Building
{
BUILDING_NONE=0,
BUILDING_SETTLER,
BUILDING_MILITIA,
BUILDING_ARCHER,
BUILDING_PHALANX,
BUILDING_TRIREME,
BUILDING_HORSEMAN,
BUILDING_CATAPULT,
BUILDING_KNIGHT,
BUILDING_SAIL,
BUILDING_UNIT_MAX=BUILDING_SAIL,
BUILDING_GRANARY,
BUILDING_BARRACKS,
BUILDING_CITYWALLS,
BUILDING_MARKET,
BUILDING_BANK,
BUILDING_FACTORY,
BUILDING_AQUEDUCT,
BUILDING_MAX
};

struct BuildingXref
{
    BuildingType buildingtype;
    uint8_t building;
};

struct ImprovementData
{
    const char *name;
    uint32_t buildresources;
    uint32_t buildgold;
    uint32_t upkeepgold;
    uint32_t sellgold;
};

extern const ImprovementData improvementdata[];
extern const BuildingXref buildingxref[];
