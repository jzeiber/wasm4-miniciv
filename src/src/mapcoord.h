#pragma once

#include <stdint.h>

class MapCoord
{
public:
    MapCoord(const int32_t width, const int32_t height, const int32_t x, const int32_t y);
    ~MapCoord();

    bool Valid() const;
    int32_t X() const;
    int32_t Y() const;

    void Set(const int32_t x, const int32_t y);
    void Add(const int32_t dx, const int32_t dy);

private:
    int32_t m_width;
    int32_t m_height;
    int32_t m_x;
    int32_t m_y;

    void WrapX();

};