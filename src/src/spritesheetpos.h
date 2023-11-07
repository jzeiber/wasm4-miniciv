#pragma once

#include <stdint.h>

class SpriteSheetPos
{
public:
    SpriteSheetPos();
    SpriteSheetPos(const int32_t xidx, const int32_t yidx);
    ~SpriteSheetPos();

    void Set(const int32_t xidx, const int32_t yidx);

    int32_t m_xidx;
    int32_t m_yidx;

};
