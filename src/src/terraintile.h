#pragma once

#include "baseterrain.h"
#include "spritesheetpos.h"

class TerrainTile
{
public:
    TerrainTile();
    ~TerrainTile();

    enum TerrainType
    {
        TERRAIN_NONE=0,
        TERRAIN_DEEPWATER,
        TERRAIN_SHALLOWWATER,
        TERRAIN_BARREN,
        TERRAIN_HILL,
        TERRAIN_MOUNTAIN,
        TERRAIN_GRASSLAND,
        TERRAIN_FOREST,
        TERRAIN_SWAMP,
        TERRAIN_DESERT
    };

    static const char *TerrainDesc[];

    enum ClimateType
    {
        CLIMATE_NONE=0,
        CLIMATE_ARCTIC,
        CLIMATE_TUNDRA,
        CLIMATE_TEMPERATE,
        CLIMATE_SUBTROPICAL,
        CLIMATE_TROPICAL
    };

    static const char *ClimateDesc[];

    enum TileResource
    {
        RESOURCE_NONE=0,
        RESOURCE_COAL,
        RESOURCE_GOLD,
        RESOURCE_DIAMOND,
        RESOURCE_OIL,
        RESOURCE_FISH,
        RESOURCE_HORSE,
        RESOURCE_BISON,
        RESOURCE_SEAL
    };

    static const char *ResourceDesc[];

    struct ProductionData
    {
        int8_t food;
        int8_t resources;
    };

    static const ProductionData terrainproduction[];
    static const ProductionData resourceproduction[];

    void SetBaseType(const BaseTerrain::TerrainType basetype);
    void SetType(const TerrainType type);
    void SetClimate(const ClimateType climate);
    void SetHeight(const float height);
    void SetResource(const TileResource resource);
    void SetSpriteSheetPos(const uint8_t idx, const SpriteSheetPos &pos);

    BaseTerrain::TerrainType BaseType() const;
    TerrainType Type() const;
    ClimateType Climate() const;
    float Height() const;
    TileResource Resource() const;
    SpriteSheetPos GetSpriteSheetPos(const uint8_t idx) const;

private:

    BaseTerrain::TerrainType m_basetype;
    TerrainType m_type;
    ClimateType m_climate;
    SpriteSheetPos m_sprites[6];                // layers of sprites if needed, back to front
    float m_height;
    TileResource m_resource;

};
