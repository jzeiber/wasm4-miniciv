#pragma once

#include <stdint.h>
#include "simplexnoise.h"

// basic noise for land/water
class BaseTerrain
{
public:
    BaseTerrain();
    ~BaseTerrain();

    enum TerrainType:uint8_t
    {
        BASETERRAIN_NONE=0,
        BASETERRAIN_LAND=1,
        BASETERRAIN_WATER=2
    };

    void SetNoise(SimplexNoise *noise);
    void SetSize(const int32_t width, const int32_t height);

    float GetTerrainHeight(const int32_t x, const int32_t y) const;
    TerrainType GetTerrainType(const int32_t x, const int32_t y) const;

private:

    SimplexNoise *m_noise;
    int32_t m_width;
    int32_t m_height;

};
