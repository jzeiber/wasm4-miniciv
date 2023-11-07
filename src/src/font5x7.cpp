#include "Font5x7.h"
#include "spritefont5x7.h"
#include "wasm4.h"

Font5x7::Font5x7()
{

}

Font5x7::~Font5x7()
{

}

Font5x7 &Font5x7::Instance()
{
    static Font5x7 f;
    return f;
}

int16_t Font5x7::CharHeight() const
{
    return 7;
}

int16_t Font5x7::LineHeight() const
{
    return 8;
}

int16_t Font5x7::CharWidth(const char c) const
{
    // space
    if(c==32)
    {
        return 5;
    }
    // look for rightmost set bit in glyph and add 1
    if(c>=32 && c<128)
    {
        const int16_t cx=(c%16)*7;
        const int16_t cy=((c/16)*8)-16;
        for(int16_t dx=6; dx>=0; dx--)
        {
            for(int16_t dy=0; dy<8; dy++)
            {
                const int16_t bitpos=((cy+dy)*112)+(cx+dx);
                const int16_t spritebyte=bitpos/8;
                const int16_t spritebit=bitpos%8;
                if(spritefont5x7[spritebyte] >> (7-spritebit) & 0x01 == 1)
                {
                    return dx+2;
                }                
            }
        }
        /*
        for(int16_t dy=0; dy<10; dy++)
        {
            for(int16_t dx=maxx; dx<8; dx++)
            {

                if(maxx<dx && spritefont5x7[spritebyte] >> (7-spritebit) & 0x01 == 1)
                {
                    maxx=dx;
                }
            }
        }
        return maxx+2;
        */
    }
    return 6;
}

int16_t Font5x7::TextWidth(const char *text) const
{
    int16_t pos=0;
    int16_t w=0;
    while(text && text[pos]!='\0')
    {
        w+=CharWidth(text[pos]);
        pos++;
    }
    return w;
}

void Font5x7::PutChar(const char c, const int16_t x, const int16_t y, const uint16_t color) const
{
    *DRAW_COLORS=(color << 4);
    if(c>=32 && c<128)
    {
        const int16_t cx=(c%16)*7;
        const int16_t cy=((c/16)*8)-16;
        blitSub(spritefont5x7,x,y,7,8,cx,cy,spritefont5x7Width,spritefont5x7Flags);
    }
}
