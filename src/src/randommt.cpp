#include "randommt.h"

RandomMT::RandomMT()
{
	tinymt64_init(&m_state,0x1);
}

RandomMT::RandomMT(const uint64_t seed)
{
	Seed(seed);
}

RandomMT::~RandomMT()
{

}

RandomMT &RandomMT::Instance()
{
	static RandomMT r;
	return r;
}
	
void RandomMT::Seed(const uint64_t seed)
{
	// init does not initialze context to 0, so do it here
	m_state.status[0]=0;
	m_state.status[1]=0;
	m_state.mat1=0;
	m_state.mat2=0;
	m_state.tmat=0;
	tinymt64_init(&m_state,seed);
}

uint64_t RandomMT::Next()
{
	return tinymt64_generate_uint64(&m_state);
}

double RandomMT::NextDouble()
{
	return tinymt64_generate_double(&m_state);
}

double RandomMT::NextGaussianDouble()
{
	return (tinymt64_generate_double(&m_state)+tinymt64_generate_double(&m_state)+tinymt64_generate_double(&m_state)+tinymt64_generate_double(&m_state))/4.0;
}
