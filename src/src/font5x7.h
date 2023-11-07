#pragma once

#include "ifont.h"

class Font5x7:public IFont
{
public:
    Font5x7();
    ~Font5x7();

    static Font5x7 &Instance();

    virtual int16_t CharHeight() const;
    virtual int16_t LineHeight() const;
    virtual int16_t CharWidth(const char c) const;
    virtual int16_t TextWidth(const char *text) const;

    virtual void PutChar(const char c, const int16_t x, const int16_t y, const uint16_t color) const;

};