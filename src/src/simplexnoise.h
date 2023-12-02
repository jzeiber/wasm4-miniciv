#pragma once

#include <stddef.h>
#include <stdint.h>

class SimplexNoise
{
public:
	SimplexNoise();
	~SimplexNoise();

	float Noise(const float x, const float y) const;

	float Fractal(const size_t octaves, const float frequency, const float amplitude, const float lacunarity, const float persistence, const float x, const float y) const;
	float FractalWrappedWidth(const size_t octaves, const float frequency, const float amplitude, const float lacunarity, const float persistence, const float x, const float y, const float width, const float height) const;
	float FractalWrappedHeight(const size_t octaves, const float frequency, const float amplitude, const float lacunarity, const float persistence, const float x, const float y, const float width, const float height) const;

	void Seed(const uint64_t seed);

private:

	//uint8_t m_perm[256];
	uint8_t *m_perm;

	int32_t FastFloor(float fp) const;
	uint8_t Hash(const int32_t i) const;
	float Grad(const int32_t hash, const float x, const float y) const;

};
