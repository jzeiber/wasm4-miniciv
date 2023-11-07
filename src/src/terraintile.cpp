#include "terraintile.h"
#include "cppfuncs.h"

const char *TerrainTile::TerrainDesc[]={
"None",
"Deep Water",
"Coastal Water",
"Barren",
"Hill",
"Mountain",
"Grassland",
"Forest",
"Swamp",
"Desert"
};

const char *TerrainTile::ClimateDesc[]={
"None",
"Arctic",
"Tundra",
"Temperate",
"Subtropical",
"Tropical"
};

const char *TerrainTile::ResourceDesc[]={
"None",
"Coal",
"Gold",
"Diamond",
"Oil",
"Fish",
"Horse",
"Bison",
"Seal"
};

const TerrainTile::ProductionData TerrainTile::terrainproduction[]={
// ID,                      food,res
{/*TERRAIN_NONE,*/          0,  0},
{/*TERRAIN_DEEPWATER,*/     0,  1},
{/*TERRAIN_SHALLOWWATER,*/  1,  1},
{/*TERRAIN_BARREN,*/        0,  0},
{/*TERRAIN_HILL,*/          0,  1},
{/*TERRAIN_MOUNTAIN,*/      0,  2},
{/*TERRAIN_GRASSLAND,*/     2,  1},
{/*TERRAIN_FOREST,*/        1,  1},
{/*TERRAIN_SWAMP,*/         1,  0},
{/*TERRAIN_DESERT,*/        0,  1}
};

const TerrainTile::ProductionData TerrainTile::resourceproduction[]={
// ID,                   food,res
{/*RESOURCE_NONE,*/         0,  0},
{/*RESOURCE_COAL,*/         0,  1},
{/*RESOURCE_GOLD,*/         0,  3},
{/*RESOURCE_DIAMOND,*/      0,  3},
{/*RESOURCE_OIL,*/          0,  2},
{/*RESOURCE_FISH,*/         2,  0},
{/*RESOURCE_HORSE,*/        0,  1},
{/*RESOURCE_BISON,*/        1,  1},
{/*RESOURCE_SEAL*/          1,  1}
};

TerrainTile::TerrainTile():m_basetype(BaseTerrain::BASETERRAIN_NONE),m_type(TerrainTile::TERRAIN_NONE),m_climate(TerrainTile::CLIMATE_NONE),m_height(__FLT_MIN__),m_resource(TerrainTile::RESOURCE_NONE)
{

}

TerrainTile::~TerrainTile()
{

}

void TerrainTile::SetBaseType(const BaseTerrain::TerrainType basetype)
{
    m_basetype=basetype;
}

void TerrainTile::SetType(const TerrainType type)
{
    m_type=type;
}

void TerrainTile::SetClimate(const ClimateType climate)
{
    m_climate=climate;
}

void TerrainTile::SetHeight(const float height)
{
    m_height=height;
}

void TerrainTile::SetResource(const TileResource resource)
{
    m_resource=resource;
}

void TerrainTile::SetSpriteSheetPos(const uint8_t idx, const SpriteSheetPos &pos)
{
    if(idx>=0 && idx<countof(m_sprites))
    {
        m_sprites[idx]=pos;
    }
}

BaseTerrain::TerrainType TerrainTile::BaseType() const
{
    return m_basetype;
}

TerrainTile::TerrainType TerrainTile::Type() const
{
    return m_type;
}

TerrainTile::ClimateType TerrainTile::Climate() const
{
    return m_climate;
}

float TerrainTile::Height() const
{
    return m_height;
}

TerrainTile::TileResource TerrainTile::Resource() const
{
    return m_resource;
}

SpriteSheetPos TerrainTile::GetSpriteSheetPos(const uint8_t idx) const
{
    if(idx>=0 && idx<countof(m_sprites))
    {
        return m_sprites[idx];
    }
    return SpriteSheetPos();
}
