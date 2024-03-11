#include "map.h"
#include "mapcoord.h"
#include "global.h"
#include "cppfuncs.h"
#include "randommt.h"

struct masktilepos
{
    uint8_t mask;
    uint8_t xidx;
    uint8_t yidx;
    bool diag;      // include diagonals in mask check
};

const masktilepos shallowtilepos[]={
{DIR_NORTH | DIR_EAST | DIR_SOUTH | DIR_WEST,                       7,  1, false},
{DIR_EAST | DIR_SOUTH,                                              6,  0, false},  // non shallow water to north and west
{DIR_SOUTH | DIR_WEST,                                              8,  0, false},  // non shallow water to north and east
{DIR_EAST | DIR_NORTH,                                              6,  2, false},  // non shallow water to south and west
{DIR_NORTH | DIR_WEST,                                              8,  2, false},  // non shallow water to east and south
{DIR_EAST | DIR_SOUTH | DIR_WEST,                                   7,  0, false},  // non shallow water to north only
{DIR_NORTH | DIR_EAST | DIR_SOUTH,                                  6,  1, false},  // non shallow water to west only
{DIR_NORTH | DIR_SOUTH | DIR_WEST,                                  8,  1, false},  // non shallow weter to east only
{DIR_NORTH | DIR_EAST | DIR_WEST,                                   7,  2, false},  // non shallow water to south only
{DIR_NONE,                                                          11, 0, false}   // surrounded by deep water
};

const masktilepos landtilepos[]={
{DIR_NORTH | DIR_EAST | DIR_SOUTH | DIR_WEST,                       1,  1, false},
{DIR_EAST | DIR_SOUTH,                                              0,  0, false},  // water to north and west
{DIR_SOUTH | DIR_WEST,                                              2,  0, false},  // water to north and east
{DIR_EAST | DIR_NORTH,                                              0,  2, false},  // water to south and west
{DIR_NORTH | DIR_WEST,                                              2,  2, false},  // water to east and south
{DIR_EAST | DIR_SOUTH | DIR_WEST,                                   1,  0, false},  // water to north only
{DIR_NORTH | DIR_EAST | DIR_SOUTH,                                  0,  1, false},  // water to west only
{DIR_NORTH | DIR_SOUTH | DIR_WEST,                                  2,  1, false},  // weter to east only
{DIR_NORTH | DIR_EAST | DIR_WEST,                                   1,  2, false},  // water to south only
{DIR_NORTH | DIR_SOUTH,                                             3,  1, false},  // water to east and west
{DIR_EAST | DIR_WEST,                                               4,  1, false},  // water to north and south
{DIR_SOUTH,                                                         3,  0, false},  // water to north, east, west
{DIR_NORTH,                                                         3,  2, false},  // water to east, south, west
{DIR_EAST,                                                          4,  0, false},  // water to north, south, west
{DIR_WEST,                                                          4,  2, false},  // water to north, east, south
{DIR_NONE,                                                          5,  0, false}   // surrounded by water
};

struct resourceplacementdata
{
    TerrainTile::TileResource resource;
    SpriteSheetPos spritesheetpos;
    BaseTerrain::TerrainType baseterrain;
    TerrainTile::TerrainType terrain;
    TerrainTile::ClimateType climate;
    bool inland;
    bool coast;
    bool shallow;
    bool deep;
    uint64_t extraseed;
    float chance;
};

const resourceplacementdata resourceplacement[]={
//resource,                         spritesheetpos,         baseterrain,                    terrain,                        climate,                            inland,     coast,  shallow,    deep,   extraseed,              chance
{TerrainTile::RESOURCE_SEAL,        SpriteSheetPos(7,6),    BaseTerrain::BASETERRAIN_NONE,  TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_ARCTIC,        false,      true,   true,       false,  8495720930874635291,    0.01},
{TerrainTile::RESOURCE_BISON,       SpriteSheetPos(6,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_TUNDRA,        true,       true,   false,      false,  264959276453320123,     0.05},
{TerrainTile::RESOURCE_FISH,        SpriteSheetPos(4,6),    BaseTerrain::BASETERRAIN_WATER, TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_TUNDRA,        false,      false,  true,       true,   8473648509388462109,    0.01},
{TerrainTile::RESOURCE_FISH,        SpriteSheetPos(4,6),    BaseTerrain::BASETERRAIN_WATER, TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_TEMPERATE,     false,      false,  true,       true,   8473648509388462109,    0.02},
{TerrainTile::RESOURCE_FISH,        SpriteSheetPos(4,6),    BaseTerrain::BASETERRAIN_WATER, TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_SUBTROPICAL,   false,      false,  true,       true,   8473648509388462109,    0.03},
{TerrainTile::RESOURCE_FISH,        SpriteSheetPos(4,6),    BaseTerrain::BASETERRAIN_WATER, TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_TROPICAL,      false,      false,  true,       true,   8473648509388462109,    0.03},
// shallow water fish get a little extra chance
{TerrainTile::RESOURCE_FISH,        SpriteSheetPos(4,6),    BaseTerrain::BASETERRAIN_WATER, TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_TUNDRA,        false,      false,  true,       false,  8473648509388462109,    0.02},
{TerrainTile::RESOURCE_FISH,        SpriteSheetPos(4,6),    BaseTerrain::BASETERRAIN_WATER, TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_TEMPERATE,     false,      false,  true,       false,  8473648509388462109,    0.03},
{TerrainTile::RESOURCE_FISH,        SpriteSheetPos(4,6),    BaseTerrain::BASETERRAIN_WATER, TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_SUBTROPICAL,   false,      false,  true,       false,  8473648509388462109,    0.04},
{TerrainTile::RESOURCE_FISH,        SpriteSheetPos(4,6),    BaseTerrain::BASETERRAIN_WATER, TerrainTile::TERRAIN_NONE,      TerrainTile::CLIMATE_TROPICAL,      false,      false,  true,       false,  8473648509388462109,    0.04},
{TerrainTile::RESOURCE_COAL,        SpriteSheetPos(0,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_TUNDRA,        true,       true,   false,      false,  1473940838746110293,    0.05},
{TerrainTile::RESOURCE_COAL,        SpriteSheetPos(0,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_TUNDRA,        true,       true,   false,      false,  1473940838746110293,    0.05},
{TerrainTile::RESOURCE_COAL,        SpriteSheetPos(0,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_TEMPERATE,     true,       true,   false,      false,  1473940838746110293,    0.05},
{TerrainTile::RESOURCE_COAL,        SpriteSheetPos(0,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_TEMPERATE,     true,       true,   false,      false,  1473940838746110293,    0.05},
{TerrainTile::RESOURCE_COAL,        SpriteSheetPos(0,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_SUBTROPICAL,   true,       true,   false,      false,  1473940838746110293,    0.05},
{TerrainTile::RESOURCE_COAL,        SpriteSheetPos(0,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_SUBTROPICAL,   true,       true,   false,      false,  1473940838746110293,    0.05},
{TerrainTile::RESOURCE_COAL,        SpriteSheetPos(0,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_TROPICAL,      true,       true,   false,      false,  1473940838746110293,    0.05},
{TerrainTile::RESOURCE_COAL,        SpriteSheetPos(0,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_TROPICAL,      true,       true,   false,      false,  1473940838746110293,    0.05},
{TerrainTile::RESOURCE_GOLD,        SpriteSheetPos(1,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_TUNDRA,        true,       true,   false,      false,  7478394826102946494,    0.01},
{TerrainTile::RESOURCE_GOLD,        SpriteSheetPos(1,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_TUNDRA,        true,       true,   false,      false,  7478394826102946494,    0.01},
{TerrainTile::RESOURCE_GOLD,        SpriteSheetPos(1,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_TEMPERATE,     true,       true,   false,      false,  7478394826102946494,    0.01},
{TerrainTile::RESOURCE_GOLD,        SpriteSheetPos(1,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_TEMPERATE,     true,       true,   false,      false,  7478394826102946494,    0.01},
{TerrainTile::RESOURCE_GOLD,        SpriteSheetPos(1,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_SUBTROPICAL,   true,       true,   false,      false,  7478394826102946494,    0.01},
{TerrainTile::RESOURCE_GOLD,        SpriteSheetPos(1,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_SUBTROPICAL,   true,       true,   false,      false,  7478394826102946494,    0.01},
{TerrainTile::RESOURCE_GOLD,        SpriteSheetPos(1,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_TROPICAL,      true,       true,   false,      false,  7478394826102946494,    0.01},
{TerrainTile::RESOURCE_GOLD,        SpriteSheetPos(1,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_TROPICAL,      true,       true,   false,      false,  7478394826102946494,    0.01},
{TerrainTile::RESOURCE_DIAMOND,     SpriteSheetPos(2,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_SUBTROPICAL,   true,       true,   false,      false,  3648499203016374839,    0.01},
{TerrainTile::RESOURCE_DIAMOND,     SpriteSheetPos(2,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_SUBTROPICAL,   true,       true,   false,      false,  3648499203016374839,    0.01},
{TerrainTile::RESOURCE_DIAMOND,     SpriteSheetPos(2,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_MOUNTAIN,  TerrainTile::CLIMATE_TROPICAL,      true,       true,   false,      false,  3648499203016374839,    0.01},
{TerrainTile::RESOURCE_DIAMOND,     SpriteSheetPos(2,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_HILL,      TerrainTile::CLIMATE_TROPICAL,      true,       true,   false,      false,  3648499203016374839,    0.01},
{TerrainTile::RESOURCE_OIL,         SpriteSheetPos(3,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_GRASSLAND, TerrainTile::CLIMATE_TEMPERATE,     true,       false,  false,      false,  8747392384765629399,    0.01},
{TerrainTile::RESOURCE_OIL,         SpriteSheetPos(3,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_FOREST,    TerrainTile::CLIMATE_TEMPERATE,     true,       false,  false,      false,  8747392384765629399,    0.01},
{TerrainTile::RESOURCE_OIL,         SpriteSheetPos(3,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_GRASSLAND, TerrainTile::CLIMATE_SUBTROPICAL,   true,       false,  false,      false,  8747392384765629399,    0.01},
{TerrainTile::RESOURCE_OIL,         SpriteSheetPos(3,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_FOREST,    TerrainTile::CLIMATE_SUBTROPICAL,   true,       false,  false,      false,  8747392384765629399,    0.01},
{TerrainTile::RESOURCE_HORSE,       SpriteSheetPos(5,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_GRASSLAND, TerrainTile::CLIMATE_TEMPERATE,     true,       false,  false,      false,  4874756234874923844,    0.03},
{TerrainTile::RESOURCE_HORSE,       SpriteSheetPos(5,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_FOREST,    TerrainTile::CLIMATE_TEMPERATE,     true,       false,  false,      false,  4874756234874923844,    0.01},
{TerrainTile::RESOURCE_HORSE,       SpriteSheetPos(5,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_GRASSLAND, TerrainTile::CLIMATE_SUBTROPICAL,   true,       false,  false,      false,  4874756234874923844,    0.02},
{TerrainTile::RESOURCE_HORSE,       SpriteSheetPos(5,6),    BaseTerrain::BASETERRAIN_LAND,  TerrainTile::TERRAIN_FOREST,    TerrainTile::CLIMATE_SUBTROPICAL,   true,       false,  false,      false,  4874756234874923844,    0.01},
};

constexpr int8_t matchmask(const masktilepos *masklist, const uint8_t maskcount, const uint8_t mask)
{
    for(uint8_t i=0; i<maskcount; i++)
    {
        if((masklist[i].diag==false && (masklist[i].mask & ~DIR_DIAGONALS)==(mask & ~DIR_DIAGONALS)) || (masklist[i].diag==true && masklist[i].mask==mask))
        {
            return i;
        }
    }
    return -1;
}

Map::Map():m_baseterrain(),m_seed(0),m_width(0),m_height(0)
{
    m_baseterrain.SetNoise(global::noise);
    global::noise->Seed(m_seed);
}

Map::~Map()
{

}

void Map::SetSize(const int32_t width, const int32_t height)
{
    m_width=width;
    m_height=height;
    m_baseterrain.SetSize(width,height);
}

void Map::SetSeed(const uint64_t seed)
{
    m_seed=seed;
    global::noise->Seed(seed);
}

/*
void Map::InvalidateCache()
{
    // TODO - clear cache
}

void Map::CacheTiles(const int32_t x, const int32_t y, const int32_t width, const int32_t height)
{
    // TODO - fill cache with terrain tiles
}
*/

TerrainTile Map::GetTile(const int32_t x, const int32_t y) const
{
    // TODO - look in cache - if not there then compute tile

    return ComputeTile(x,y);
}

BaseTerrain::TerrainType Map::GetBaseType(const int32_t x, const int32_t y) const
{
    return m_baseterrain.GetTerrainType(x,y);
}

TerrainTile Map::ComputeTile(const int32_t x, const int32_t y) const
{
    TerrainTile tile;
    uint8_t spriteidx=0;
    //SimplexNoise sn;    // noise for sprite groupings
    //sn.Seed(m_seed+1);
    //const float noise=sn.FractalWrappedWidth(2,1.0/32.0,1.0,2.0,0.5,x,y,m_width,m_height);

    if(y<0 || y>=m_height)
    {
        return tile;
    }

    MapCoord coord(m_width,m_height,x,y);

    tile.SetBaseType(m_baseterrain.GetTerrainType(coord.X(),coord.Y()));
    tile.SetHeight(m_baseterrain.GetTerrainHeight(coord.X(),coord.Y()));

    ComputeTileClimate(tile,coord);

    if(tile.BaseType()==BaseTerrain::BASETERRAIN_WATER)
    {
        // all water tiles have default background of deep water (so shallow tiles next to deep water show correctly)
        tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(11,1));

        const uint8_t landmask=SurroundingTerrainMask(coord.X(),coord.Y(),BaseTerrain::BASETERRAIN_LAND);
        if(landmask==DIR_NONE)  // no land surrounding - deep water
        {
            tile.SetType(TerrainTile::TERRAIN_DEEPWATER);
        }
        else
        {
            tile.SetType(TerrainTile::TERRAIN_SHALLOWWATER);
            // determine specific sprite
            const uint8_t shallowmask=ShallowWaterMask(coord.X(),coord.Y());
            const int8_t mm=matchmask(shallowtilepos,countof(shallowtilepos),shallowmask);
            if(mm>=0)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(shallowtilepos[mm].xidx,shallowtilepos[mm].yidx));
            }
        }
    }
    else if(tile.BaseType()==BaseTerrain::BASETERRAIN_LAND)
    {
        // TODO - finish

        // all land tiles have default background of shallow water (so land tiles next to water show correctly)
        tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(7,1));

        const uint8_t landmask=SurroundingTerrainMask(coord.X(),coord.Y(),BaseTerrain::BASETERRAIN_LAND);
        const uint8_t watermask=SurroundingTerrainMask(coord.X(),coord.Y(),BaseTerrain::BASETERRAIN_WATER);

        const int8_t lm=matchmask(landtilepos,countof(landtilepos),landmask);
        if(lm>=0)
        {
            tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(landtilepos[lm].xidx,landtilepos[lm].yidx));
        }

        // mountains
        if(tile.Height()>0.70)
        {
            const uint8_t lowercount=GetSurroundingLowerTerrainCount(coord.X(),coord.Y());
            tile.SetType(TerrainTile::TERRAIN_MOUNTAIN);
            if(tile.Height()>0.85 && lowercount>7)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(0,3));
            }
            else if(tile.Height()>0.80 && lowercount>6)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(1,3));
            }
            else if(tile.Height()>0.75 && lowercount>5)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(2,3));
            }
            else
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(3,3));
            }
        }
        else if(tile.Height()>0.6)
        {
            tile.SetType(TerrainTile::TERRAIN_HILL);
            if(tile.Height()>0.68)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(4,3));
            }
            else if(tile.Height()>0.65)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(5,3));
            }
            else if(tile.Height()>0.62)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(6,3));
            }
            else
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(7,3));
            }
        }
        else if(tile.Climate()!=TerrainTile::CLIMATE_ARCTIC && tile.Climate()!=TerrainTile::CLIMATE_TUNDRA && tile.Height()>0.5)
        {
            tile.SetType(TerrainTile::TERRAIN_FOREST);
            const int32_t sy=(tile.Climate()==TerrainTile::CLIMATE_TROPICAL || tile.Climate()==TerrainTile::CLIMATE_SUBTROPICAL) ? 5 : 4;
            if(tile.Height()>0.58)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(0,sy));
            }
            else if(tile.Height()>0.55)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(1,sy));
            }
            else if(tile.Height()>0.52)
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(2,sy));
            }
            else
            {
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(3,sy));
            }
        }
        else
        {
            const float cr=CoordRandom(coord.X(),coord.Y(),m_seed);

            // TODO - small chance of random forest in temperate, subtropical, and tropical
            if(watermask==DIR_NONE && ((tile.Climate()==TerrainTile::CLIMATE_TEMPERATE && cr<0.1) || (tile.Climate()==TerrainTile::CLIMATE_SUBTROPICAL && cr<0.05) || (tile.Climate()==TerrainTile::CLIMATE_TROPICAL && cr<0.05)))
            {
                tile.SetType(TerrainTile::TERRAIN_FOREST);
                int32_t sxidx=0;
                int32_t syidx=4;
                if(tile.Climate()==TerrainTile::CLIMATE_SUBTROPICAL || tile.Climate()==TerrainTile::CLIMATE_TROPICAL)
                {
                    syidx++;
                }
                sxidx+=static_cast<int32_t>(CoordRandom(coord.X(),coord.Y(),487348734587435)*4.0);
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(sxidx,syidx));
            }
            // chance of swamp subtropical and tropical
            else if(watermask==DIR_NONE && cr<0.2 && (tile.Climate()==TerrainTile::CLIMATE_SUBTROPICAL || tile.Climate()==TerrainTile::CLIMATE_TROPICAL))
            {
                tile.SetType(TerrainTile::TERRAIN_SWAMP);
                int32_t sxidx=4+static_cast<int32_t>(CoordRandom(coord.X(),coord.Y(),881263762364501)*4.0);
                int32_t syidx=4;
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(sxidx,syidx));
            }
            else if(tile.Climate()==TerrainTile::CLIMATE_ARCTIC)
            {
                tile.SetType(TerrainTile::TERRAIN_BARREN);

                int32_t sxidx=8;
                if(watermask)
                {
                    sxidx+=2;
                }
                if(cr<0.5)
                {
                    sxidx++;
                }
                tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(sxidx,3));
                /*
                if(CoordRandom(coord.X(),coord.Y(),m_seed)<0.5)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(3,7));
                }
                else
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(2,7));
                }
                */
            }
            else if(tile.Climate()==TerrainTile::CLIMATE_TUNDRA)
            {
                tile.SetType(TerrainTile::TERRAIN_BARREN);

                int32_t sxidx=8;
                if(watermask)
                {
                    sxidx+=2;
                }

                if(watermask==DIR_NONE && tile.Height()>=0.4 && cr<0.1)  // 10% chance of forest on high tile
                {
                    tile.SetType(TerrainTile::TERRAIN_FOREST);
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(3,4));
                }
                else
                {
                    if(cr<0.5)
                    {
                        sxidx++;
                    }
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(sxidx,4));
                }

                /*
                if(watermask==DIR_NONE)
                {
                    const float r=CoordRandom(coord.X(),coord.Y(),m_seed);
                    if(tile.Height()>=0.4 && r<0.1) // 10% chance of forest on high tile
                    {
                        tile.SetType(TerrainTile::TERRAIN_FOREST);
                        tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(3,4));
                    }
                    else
                    {
                        tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(2,10));
                    }
                }
                */
            }
            else if(tile.Climate()==TerrainTile::CLIMATE_TEMPERATE)
            {
                tile.SetType(TerrainTile::TERRAIN_GRASSLAND);

                if(cr<0.25)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(4,5));
                }
                else if(cr<0.5)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(5,5));
                }
                else if(cr<0.75)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(6,5));
                }
                else
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(7,5));
                }
            }
            else if(tile.Climate()==TerrainTile::CLIMATE_SUBTROPICAL)
            {
                tile.SetType(TerrainTile::TERRAIN_GRASSLAND);
                //tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(9,7));

                if(cr<0.25)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(4,5));
                }
                else if(cr<0.5)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(5,5));
                }
                else if(cr<0.75)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(6,5));
                }
                else
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(7,5));
                }
            }
            else if(tile.Climate()==TerrainTile::CLIMATE_TROPICAL)
            {
                tile.SetType(TerrainTile::TERRAIN_GRASSLAND);
                //tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(9,7));

                if(cr<0.25)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(4,5));
                }
                else if(cr<0.5)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(5,5));
                }
                else if(cr<0.75)
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(6,5));
                }
                else
                {
                    tile.SetSpriteSheetPos(spriteidx++,SpriteSheetPos(7,5));
                }
            }
        }

    }

    ComputeTileResource(tile,coord,spriteidx);

    return tile;

}

void Map::ComputeTileResource(TerrainTile &tile, const MapCoord &coord, uint8_t &spriteidx) const
{
    for(size_t i=0; i<countof(resourceplacement); i++)
    {

        if(resourceplacement[i].baseterrain!=BaseTerrain::BASETERRAIN_NONE && resourceplacement[i].baseterrain!=tile.BaseType())
        {
            continue;
        }
        if(resourceplacement[i].terrain!=TerrainTile::TERRAIN_NONE && resourceplacement[i].terrain!=tile.Type())
        {
            continue;
        }
        if(resourceplacement[i].climate!=TerrainTile::CLIMATE_NONE && resourceplacement[i].climate!=tile.Climate())
        {
            continue;
        }
        if(resourceplacement[i].inland==true || resourceplacement[i].coast==true || resourceplacement[i].shallow==true || resourceplacement[i].deep==true)
        {
            bool ok=false;

            if((resourceplacement[i].inland==true || resourceplacement[i].coast==true) && tile.BaseType()==BaseTerrain::BASETERRAIN_LAND)
            {
                const uint8_t watermask=SurroundingTerrainMask(coord.X(),coord.Y(),BaseTerrain::BASETERRAIN_WATER);
                // inland shouldn't have any water adjacent
                if(resourceplacement[i].inland==true && watermask==DIR_NONE)
                {
                    ok=true;
                }
                // coast must have water adjacent
                if(resourceplacement[i].coast==true && watermask!=DIR_NONE)
                {
                    ok=true;
                }
            }
            // shallow or deep water
            if((resourceplacement[i].shallow==true && tile.Type()==TerrainTile::TERRAIN_SHALLOWWATER) || (resourceplacement[i].deep==true && tile.Type()==TerrainTile::TERRAIN_DEEPWATER))
            {
                ok=true;
            }

            if(ok==false)
            {
                continue;
            }
        }

        // we got here, so everything matches as far as terrain, now roll the dice
        if(CoordChance(coord.X(),coord.Y(),resourceplacement[i].extraseed,resourceplacement[i].chance)==true)
        {
            tile.SetResource(resourceplacement[i].resource);
            tile.SetSpriteSheetPos(spriteidx++,resourceplacement[i].spritesheetpos);
            return;
        }

    }
}

void Map::ComputeTileClimate(TerrainTile &tile, const MapCoord &coord) const
{
    RandomMT rand(8345879485345+((coord.X() << 8) | coord.Y()));
    const int32_t o=(rand.NextDouble() * 5.0)-2;            // offset +/- 2 for climate crossover so it's not a straight line (random returns values exlusive of 1.0, so * 5.0 to get up to 4 integer)

    // set climate based on latitude (originally based on 128 map height - changed to 96 so values were adjusted)
    //if(coord.Y()<(12+o) || coord.Y()>(m_height-(13+o)))
    if(coord.Y()<(10+o) || coord.Y()>(m_height-(11+0)))
    {
        tile.SetClimate(TerrainTile::CLIMATE_ARCTIC);
    }
    //else if(coord.Y()<(20+o) || coord.Y()>(m_height-(21+o)))
    else if(coord.Y()<(15+o) || coord.Y()>(m_height-(16+o)))
    {
        tile.SetClimate(TerrainTile::CLIMATE_TUNDRA);
    }
    //else if(coord.Y()<(45+o) || coord.Y()>(m_height-(46+o)))
    else if(coord.Y()<(30+o) || coord.Y()>(m_height-(31+o)))
    {
        tile.SetClimate(TerrainTile::CLIMATE_TEMPERATE);
    }
    //else if(coord.Y()<(56+o) || coord.Y()>(m_height-(54+o)))
    else if(coord.Y()<(42+o) || coord.Y()>(m_height-(43+o)))
    {
        tile.SetClimate(TerrainTile::CLIMATE_SUBTROPICAL);
    }
    else
    {
        tile.SetClimate(TerrainTile::CLIMATE_TROPICAL);
    }
}

uint8_t Map::SurroundingTerrainMask(const int32_t x, const int32_t y, BaseTerrain::TerrainType type) const
{
    uint8_t mask=0;
    int32_t shift=0;
    for(int32_t yy=y-1; yy<=y+1; yy++)
    {
        for(int32_t xx=x-1; xx<=x+1; xx++)
        {
            if(xx!=x || yy!=y)
            {
                if(m_baseterrain.GetTerrainType(xx,yy)==type)
                {
                    mask|=(1 << shift);
                }
                shift++;
            }
        }
    }
    return mask;
}

uint8_t Map::ShallowWaterMask(const int32_t x, const int32_t y) const
{
    uint8_t mask=0;
    int32_t shift=0;
    for(int32_t yy=y-1; yy<=y+1; yy++)
    {
        for(int32_t xx=x-1; xx<=x+1; xx++)
        {
            if(xx!=x || yy!=y)
            {
                // check if terrain is water
                const BaseTerrain::TerrainType t=m_baseterrain.GetTerrainType(xx,yy);

                // set bit if tile is land - shallow water extends from land
                if(t==BaseTerrain::BASETERRAIN_LAND)
                {
                    mask|=(1 << shift);
                }
                else if(t==BaseTerrain::BASETERRAIN_WATER)
                {
                    // check if water has land next to it - if so then it's shallow water
                    if(SurroundingTerrainMask(xx,yy,BaseTerrain::BASETERRAIN_LAND)!=DIR_NONE)
                    {
                        mask|=(1 << shift);
                    }
                }

                shift++;
            }
        }
    }
    return mask;
}

uint8_t Map::GetSurroundingLowerTerrainCount(const int32_t x, const int32_t y) const
{
    uint8_t count=0;

    if(y>=0 && y<m_height)
    {
        // TODO - check if tile in cache
        const float h=m_baseterrain.GetTerrainHeight(x,y);

        for(int yy=y-1; yy<y+2; yy++)
        {
            for(int xx=x-1; xx<x+2; xx++)
            {
                if((xx!=x || yy!=y) && yy>=0 && yy<m_height)
                {
                    // TODO - check if tile in cache
                    const float th=m_baseterrain.GetTerrainHeight(xx,yy);
                    if(th<h)
                    {
                        count++;
                    }
                }
            }
        }
    }

    return count;
}

double Map::CoordRandom(const int32_t x, const int32_t y, const uint64_t extraseed) const
{
    RandomMT rand(~((x << 8) | y) + extraseed);
    return rand.NextDouble();
}

bool Map::CoordChance(const int32_t x, const int32_t y, const uint64_t extraseed, float chance) const
{
    return (CoordRandom(x,y,extraseed) < chance);
}

int32_t Map::Width() const
{
    return m_width;
}

int32_t Map::Height() const
{
    return m_height;
}
