#pragma once

#include <stdint.h>

class Game;

class IUpdatable
{
public:
	IUpdatable()	{ };
	virtual ~IUpdatable()	{ };
	
	virtual void Update(const int ticks, const uint8_t playerindex, Game *game=nullptr)=0;

};
