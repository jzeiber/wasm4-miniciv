#include "baseterrain.h"
#include "cppfuncs.h"

BaseTerrain::BaseTerrain():m_noise(nullptr),m_width(128),m_height(128)
{

}

BaseTerrain::~BaseTerrain()
{

}

void BaseTerrain::SetNoise(SimplexNoise *noise)
{
    m_noise=noise;
}

void BaseTerrain::SetSize(const int32_t width, const int32_t height)
{
    m_width=width;
    m_height=height;
}

float BaseTerrain::GetTerrainHeight(const int32_t x, const int32_t y) const
{
    if(y<0 || y>=m_height)
    {
        return __FLT_MIN__;
    }

    float n=m_noise->FractalWrappedWidth(2,1.0/32.0,1.0,2.0,0.5,x,y,m_width,m_height);

    // gradually cut off near poles
    if(y<8)
    {
        n*=static_cast<float>(y)/8.0;
        n-=static_cast<float>(8-y)/8.0;
    }
    if(y>(m_height-9))
    {
        n*=static_cast<float>((m_height-1)-y)/8.0;
        n-=static_cast<float>(y-(m_height-9))/8.0;
    }

    // at pole +1.0 so we have north and south landmass
    if(y==0)
    {
        n+=1.0;
    }
    else if(y==1)
    {
        n+=0.85;
    }
    else if(y==(m_height-2))
    {
        n+=0.85;
    }
    else if(y==(m_height-1))
    {
        n+=1.0;
    }

    n=wasm4::min(1.0f,wasm4::max(-1.0f,n));

    return n;
}

BaseTerrain::TerrainType BaseTerrain::GetTerrainType(const int32_t x, const int32_t y) const
{
    const float h=GetTerrainHeight(x,y);

    if(h<-1.0)
    {
        return BASETERRAIN_NONE;
    }
    else if(h<0)
    {
        return BASETERRAIN_WATER;
    }
    else
    {
        return BASETERRAIN_LAND;
    }
}
