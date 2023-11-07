#include "mapcoord.h"

MapCoord::MapCoord(const int32_t width, const int32_t height, const int32_t x, const int32_t y):m_width(width),m_height(height),m_x(x),m_y(y)
{
    WrapX();
}

MapCoord::~MapCoord()
{

}

bool MapCoord::Valid() const
{
    return (m_y>=0 && m_y<m_height && m_x>=0 && m_x<m_width);
}

int32_t MapCoord::X() const
{
    return m_x;
}

int32_t MapCoord::Y() const
{
    return m_y;
}

void MapCoord::Set(const int32_t x, const int32_t y)
{
    m_x=x;
    m_y=y;
    WrapX();
}

void MapCoord::Add(const int32_t dx, const int32_t dy)
{
    m_x+=dx;
    m_y+=dy;
    WrapX();
}

void MapCoord::WrapX()
{
    if(m_width>0)
    {
        while(m_x>=m_width)
        {
            m_x-=m_width;
        }
        while(m_x<0)
        {
            m_x+=m_width;
        }
    }
}