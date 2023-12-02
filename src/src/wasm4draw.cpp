#include "wasm4draw.h"
#include "wasm4.h"

void blitMasked(const uint8_t *spritesheet, const uint8_t *spritemask, int32_t screenx, int32_t screeny, uint32_t width, uint32_t height, uint32_t srcX, uint32_t srcY, uint32_t stride, uint32_t flags)
{
    int m11, m12, m21, m22;  // rotation/reflection matrix
    int offx, offy;          // x,y offset to sprite coords after matrix multiplication

    // matrix reference
    // https://en.wikipedia.org/wiki/Rotations_and_reflections_in_two_dimensions

    // identity matrix to start
    m11=1;
    m12=0;
    m21=0;
    m22=1;
    offx=0;
    offy=0;

    if((flags & (BLIT_FLIP_X | BLIT_FLIP_Y | BLIT_ROTATE)) == BLIT_ROTATE)  // only rotate sprite 90 degrees counter clockwise (we need -90 in relation to sprite x but since y is positive downward this works out to 90 degree rotation)
    {
        m11=0;
        m12=-1;
        m21=1;
        m22=0;
        offx=(height-1);
        offy=0;
    }
    else if((flags & (BLIT_FLIP_X | BLIT_FLIP_Y | BLIT_ROTATE)) == BLIT_FLIP_X)  // only reflect x
    {
        m11=-1;
        m12=0;
        m21=0;
        m22=1;
        offx=(width-1);
        offy=0;
    }
    else if((flags & (BLIT_FLIP_X | BLIT_FLIP_Y | BLIT_ROTATE)) == BLIT_FLIP_Y)  // only reflect y
    {
        m11=1;
        m12=0;
        m21=0;
        m22=-1;
        offx=0;
        offy=(height-1);
    }
    else if((flags & (BLIT_FLIP_X | BLIT_FLIP_Y | BLIT_ROTATE)) == (BLIT_FLIP_X | BLIT_FLIP_Y))  // reflect x and y = 180 degree rotation
    {
        m11=-1;
        m12=0;
        m21=0;
        m22=-1;
        offx=(width-1);
        offy=(height-1);
    }
    else if((flags & (BLIT_FLIP_X | BLIT_FLIP_Y | BLIT_ROTATE)) == (BLIT_ROTATE | BLIT_FLIP_X))   // reflect x and rotate = 45 degree reflection
    {
        m11=0;
        m12=1;
        m21=1;
        m22=0;
        offx=0;
        offy=0;
    }
    else if((flags & (BLIT_FLIP_X | BLIT_FLIP_Y | BLIT_ROTATE)) == (BLIT_ROTATE | BLIT_FLIP_Y))     // reflect y and rotate = -45 degree reflection
    {
        m11=0;
        m12=-1;
        m21=-1;
        m22=0;
        offx=(height-1);
        offy=(width-1);
    }
    else if((flags & (BLIT_FLIP_X | BLIT_FLIP_Y | BLIT_ROTATE)) == (BLIT_ROTATE | BLIT_FLIP_X | BLIT_FLIP_Y))   // reflect x and y and rotate = rotate 90 degrees clockwise
    {
        m11=0;
        m12=1;
        m21=-1;
        m22=0;
        offx=0;
        offy=(width-1);
    }

    for(int32_t sy=0; sy<height; sy++)
    {
        for(int32_t sx=0; sx<width; sx++)
        {
            if(screenx+sx>=0 && screeny+sy>=0 && screenx+sx<SCREEN_SIZE && screeny+sy<SCREEN_SIZE)
            {
                const int32_t sheetx=srcX+offx+(m11*sx)+(m12*sy);
                const int32_t sheety=srcY+offy+(m21*sx)+(m22*sy);

                if(sheetx>=0 && sheety>=0 && sheetx<stride)
                {
                    const int32_t maskbitpos=(sheety*stride)+(sheetx);
                    const int32_t maskbit=7-(maskbitpos%8);
                    const int32_t maskbyte=maskbitpos/8;

                    if(((spritemask[maskbyte] >> maskbit) & 0x1))
                    {
                        // framebuffer position
                        const int32_t fbbitpos=((screeny+sy)*SCREEN_SIZE*2)+((screenx+sx)*2);
                        const int32_t fbbit=(fbbitpos%8);
                        const int32_t fbbyte=fbbitpos/8;

                        // BLIT_1BPP is 0 - so we need to check for 2BPP flag
                        const int32_t bpp=((flags & BLIT_2BPP) ? 2 : 1);
                        const int32_t sheetbitpos=(sheety*stride*bpp)+(sheetx*bpp);
                        const int32_t sheetbit=(8-bpp)-(sheetbitpos%8);
                        const int32_t sheetbyte=sheetbitpos/8;

                        const int32_t col=(bpp==2 ? spritesheet[sheetbyte] >> sheetbit : ((spritesheet[sheetbyte] >> sheetbit) & 0x01 ? (*DRAW_COLORS >> 4)-1 : (*DRAW_COLORS)-1));
                        FRAMEBUFFER[fbbyte]=(FRAMEBUFFER[fbbyte] & ~(0x3 << fbbit)) | ((col & 0x3) << fbbit);
                    }
                }
            }
        }
    }
}
