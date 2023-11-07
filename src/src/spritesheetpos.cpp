#include "spritesheetpos.h"

SpriteSheetPos::SpriteSheetPos():m_xidx(-1),m_yidx(-1)
{

}

SpriteSheetPos::SpriteSheetPos(const int32_t xidx, const int32_t yidx):m_xidx(xidx),m_yidx(yidx)
{

}

SpriteSheetPos::~SpriteSheetPos()
{

}

void SpriteSheetPos::Set(const int32_t xidx, const int32_t yidx)
{
    m_xidx=xidx;
    m_yidx=yidx;
}