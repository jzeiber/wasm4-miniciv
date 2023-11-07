#include "terraincache.h"

TerrainCache::TerrainCache():m_startx(0),m_starty(0),m_width(0),m_height(0),m_cache(nullptr)
{

}

TerrainCache::~TerrainCache()
{
    if(m_cache)
    {
        delete [] m_cache;
    }
}
