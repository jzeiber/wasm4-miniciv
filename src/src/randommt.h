#pragma once

#include "tinymt64.h"

#include <stdint.h>

class RandomMT
{
public:
	RandomMT();
	RandomMT(const uint64_t seed);
	~RandomMT();
	
	static RandomMT &Instance();
	
	void Seed(const uint64_t seed);
	uint64_t Next();
	double NextDouble();
	double NextGaussianDouble();
	
private:
	tinymt64_t m_state;
};
