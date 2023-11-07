#include "fontsystem.h"
#include "wasm4.h"

FontSystem::FontSystem()
{

}

FontSystem::~FontSystem()
{

}

FontSystem &FontSystem::Instance()
{
    static FontSystem fs;
    return fs;
}

int16_t FontSystem::CharHeight() const
{
    return 8;
}

int16_t FontSystem::LineHeight() const
{
    return 8;
}

int16_t FontSystem::CharWidth(const char c) const
{
    return 8;
}

int16_t FontSystem::TextWidth(const char *text) const
{
    int16_t w=0;
    while(text && text[w]!='\0')
    {
        w++;
    }
    return w*8;
}

void FontSystem::PutChar(const char c, const int16_t x, const int16_t y, const uint16_t color) const
{
    *DRAW_COLORS=color;
    char t[2]={c,'\0'};
    text(t,x,y);
}
