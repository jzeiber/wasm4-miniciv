#pragma once

#include "ifont.h"

class FontSystem:public IFont
{
public:
    FontSystem();
    ~FontSystem();

    static FontSystem &Instance();

    virtual int16_t CharHeight() const;
    virtual int16_t LineHeight() const;
    virtual int16_t CharWidth(const char c) const;
    virtual int16_t TextWidth(const char *text) const;

    virtual void PutChar(const char c, const int16_t x, const int16_t y, const uint16_t color) const;

};