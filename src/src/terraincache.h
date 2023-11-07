#pragma once

#include "terraintile.h"

class TerrainCache
{
public:
    TerrainCache();
    ~TerrainCache();

private:

    int32_t m_startx;
    int32_t m_starty;
    int32_t m_width;
    int32_t m_height;

    TerrainTile *m_cache;

};
