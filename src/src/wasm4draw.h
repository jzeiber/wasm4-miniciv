#pragma once
/*
	ver 2023-10-30
*/

#include <stdint.h>

// spritesheet can be 1bpp or 2bpp, spritemask must be 1bpp with same dimensions
void blitMasked(const uint8_t *spritesheet, const uint8_t *spritemask, int32_t screenx, int32_t screeny, uint32_t width, uint32_t height, uint32_t srcX, uint32_t srcY, uint32_t stride, uint32_t flags);
