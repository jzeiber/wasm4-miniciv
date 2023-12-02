#include "simplexnoise.h"

#include "randommt.h"

// save some cart size by allocating at runtime
SimplexNoise::SimplexNoise()/*:m_perm{
    151, 160, 137, 91, 90, 15,
    131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
    190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
    88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
    77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
    102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
    135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
    5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
    223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
    129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
    251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
    49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
    138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
}*/
{
	m_perm=new uint8_t[256];
	for(size_t i=0; i<256; i++)
	{
		m_perm[i]=i;
	}
}

SimplexNoise::~SimplexNoise()
{
	delete [] m_perm;
}

float SimplexNoise::Noise(const float x, const float y) const
{
	float n0, n1, n2;
	
	const float F2=0.366025403f;
	const float G2=0.211324865f;

	const float s=(x+y)*F2;
	const float xs=x+s;
	const float ys=y+s;
	const int32_t i=FastFloor(xs);
	const int32_t j=FastFloor(ys);

	const float t=static_cast<float>(i+j)*G2;
	const float X0=i-t;
	const float Y0=j-t;
	const float x0=x-X0;
	const float y0=y-Y0;

	int32_t i1, j1;
	if(x0>y0)
	{
		i1=1;
		j1=0;
	}
	else
	{
		i1=0;
		j1=1;
	}

	const float x1=x0-i1+G2;
	const float y1=y0-j1+G2;
	const float x2=x0-1.0f+2.0f*G2;
	const float y2=y0-1.0f+2.0f*G2;

	const int gi0=Hash(i+Hash(j));
	const int gi1=Hash(i+i1+Hash(j+j1));
	const int gi2=Hash(i+1+Hash(j+1));

	float t0=0.5f-x0*x0-y0*y0;
	if(t0<0.0f)
	{
		n0=0.0f;
	}
	else
	{
		t0*=t0;
		n0=t0*t0*Grad(gi0,x0,y0);
	}

	float t1=0.5f-x1*x1-y1*y1;
	if(t1<0.0f)
	{
		n1=0.0f;
	}
	else
	{
		t1*=t1;
		n1=t1*t1*Grad(gi1,x1,y1);
	}

	float t2=0.5f-x2*x2-y2*y2;
	if(t2<0.0f)
	{
		n2=0.0f;
	}
	else
	{
		t2*=t2;
		n2=t2*t2*Grad(gi2,x2,y2);
	}

	return 45.23065f*(n0+n1+n2);
}

float SimplexNoise::Fractal(const size_t octaves, const float frequency, const float amplitude, const float lacunarity, const float persistence, const float x, const float y) const
{
	if(octaves<1)
	{
		return 0.0f;
	}

	float output=0.0f;
	float denom=0.0f;
	float freq=frequency;
	float amp=amplitude;

	for(size_t i=0; i<octaves; i++)
	{
		output+=(amp*Noise(x*freq,y*freq));
		denom+=amp;
		freq*=lacunarity;
		amp*=persistence;
	}

	return output/denom;
}

float SimplexNoise::FractalWrappedWidth(const size_t octaves, const float frequency, const float amplitude, const float lacunarity, const float persistence, const float x, const float y, const float width, const float height) const
{
	if(octaves<1 || width<=0.0 || height<=0.0)
	{
		return 0.0f;
	}

	float x1=x;
	float y1=y;
	while(x1<0.0)
	{
		x1+=width;
	}
	while(x1>=width)
	{
		x1-=width;
	}
	while(y1<0.0)
	{
		y1+=height;
	}
	while(y1>=height)
	{
		y1-=height;
	}

	const float n1=(x1/width)*Fractal(octaves,frequency,amplitude,lacunarity,persistence,x1,y1);
	const float n2=(1.0-(x1/width))*Fractal(octaves,frequency,amplitude,lacunarity,persistence,x1+width,y1);

	return n1+n2;

}

float SimplexNoise::FractalWrappedHeight(const size_t octaves, const float frequency, const float amplitude, const float lacunarity, const float persistence, const float x, const float y, const float width, const float height) const
{
	if(octaves<1 || width<=0.0 || height<=0.0)
	{
		return 0.0f;
	}

	float x1=x;
	float y1=y;
	while(x1<0.0)
	{
		x1+=width;
	}
	while(x1>=width)
	{
		x1-=width;
	}
	while(y1<0.0)
	{
		y1+=height;
	}
	while(y1>=height)
	{
		y1-=height;
	}

	const float n1=(y1/height)*Fractal(octaves,frequency,amplitude,lacunarity,persistence,x1,y1);
	const float n2=(1.0-(y1/height))*Fractal(octaves,frequency,amplitude,lacunarity,persistence,x1,y1+height);

	return n1+n2;
}

int32_t SimplexNoise::FastFloor(const float fp) const
{
	int32_t i=static_cast<int32_t>(fp);
	return (fp<i) ? (i-1) : i;
}

uint8_t SimplexNoise::Hash(const int32_t i) const
{
	return m_perm[static_cast<uint8_t>(i)];
}

float SimplexNoise::Grad(const int32_t hash, const float x, const float y) const
{
	const int32_t h=(hash & 0x3F);
	const float u=h<4 ? x : y;
	const float v=h<4 ? y : x;
	return ((h&1) ? -u : u) + ((h&2) ? -2.0f * v : 2.0f * v);
}

void SimplexNoise::Seed(const uint64_t seed)
{
	RandomMT rnd(seed);
	for(int i=0; i<256; i++)
	{
		int j=rnd.Next()%256;
		uint8_t temp=m_perm[i];
		m_perm[i]=m_perm[j];
		m_perm[j]=temp;
	}
}
