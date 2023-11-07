#pragma once

#include <stdint.h>
#include "ifont.h"

class TextPrinter
{
public:
    TextPrinter();
    ~TextPrinter();

    enum Justification
    {
        JUSTIFY_LEFT=0,
        JUSTIFY_RIGHT=1
    };

    void SetCustomFont(IFont *font);
    void SetSystemFont();

    int16_t Print(const char *text, const int16_t x, const int16_t y, const int16_t len, const uint16_t color) const;
    int16_t PrintWrapped(const char *text, const int16_t x, const int16_t y, const int16_t len, const int16_t maxwidth, const uint16_t color, const uint8_t justification=JUSTIFY_LEFT) const;
    void PrintCentered(const char *text, const int16_t cx, const int16_t y, const int16_t len, const uint16_t color) const;
    int16_t WrapPos(const char *text, const int16_t maxwidth) const;
    int16_t LineHeight() const;
/*
    void SetClipBox(const int16_t left, const int16_t top, const int16_t right, const int16_t bottom);
    void ClearClipBox();
*/
private:
    IFont *m_font;/*
    int16_t m_clipleft;
    int16_t m_cliptop;
    int16_t m_clipright;
    int16_t m_clipbottom;*/

    void PutChar(const char c, const int16_t x, const int16_t y, const uint16_t color) const;
    int16_t TextWidth(const char *text, const int16_t len) const;
};