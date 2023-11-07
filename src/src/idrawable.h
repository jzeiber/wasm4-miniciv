#pragma once

#include <stdint.h>

class IDrawable
{
public:
	IDrawable()	{ };
	virtual ~IDrawable()	{ };
	
	virtual void Draw(const uint8_t playerindex)=0;
};
