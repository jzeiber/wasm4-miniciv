#pragma once

#include <stdint.h>

class IFont
{
public:
    IFont()             { };
    virtual ~IFont()    { };

    virtual int16_t CharHeight() const=0;
    virtual int16_t LineHeight() const=0;
    virtual int16_t CharWidth(const char c) const=0;
    virtual int16_t TextWidth(const char *text) const=0;

    virtual void PutChar(const char c, const int16_t x, const int16_t y, const uint16_t color) const=0;

};
