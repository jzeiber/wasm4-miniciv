#pragma once

#include "baseterrain.h"
#include "terraintile.h"
#include "terraincache.h"
#include "mapcoord.h"

class Map
{
public:
    Map();
    ~Map();

    void SetSize(const int32_t width, const int32_t height);
    void SetSeed(const uint64_t seed);

    void InvalidateCache();
    void CacheTiles(const int32_t x, const int32_t y, const int32_t width, const int32_t height);

    TerrainTile GetTile(const int32_t x, const int32_t y) const;
    BaseTerrain::TerrainType GetBaseType(const int32_t x, const int32_t y) const;

    int32_t Width() const;
    int32_t Height() const;

private:

    BaseTerrain m_baseterrain;
    TerrainCache m_cache;
    uint64_t m_seed;
    int32_t m_width;
    int32_t m_height;

    TerrainTile ComputeTile(const int32_t x, const int32_t y) const;
    void ComputeTileResource(TerrainTile &tile, const MapCoord &coord, uint8_t &spriteidx) const;
    void ComputeTileClimate(TerrainTile &tile, const MapCoord &coord) const;
    uint8_t SurroundingTerrainMask(const int32_t x, const int32_t y, BaseTerrain::TerrainType type) const;
    uint8_t ShallowWaterMask(const int32_t x, const int32_t y) const;
    uint8_t GetSurroundingLowerTerrainCount(const int32_t x, const int32_t y) const;
    double CoordRandom(const int32_t x, const int32_t y, const uint64_t extraseed) const;
    bool CoordChance(const int32_t x, const int32_t y, const uint64_t extraseed, float chance) const;

};
