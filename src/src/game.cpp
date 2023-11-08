#include "game.h"
#include "statestartup.h"
#include "statemainmenu.h"
#include "statepregame.h"
#include "stategame.h"
#include "wasm4.h"
#include "global.h"
#include "netfuncs.h"
#include "cppfuncs.h"
#include "unitdata.h"
#include "wasmstring.h"
#include "improvementdata.h"
#include "randommt.h"
#include "palette.h"
#include "wasm4draw.h"
#include "sprites.h"

//debug
#include "outputstringstream.h"

Game::Game()
{

	for(size_t i=0; i<countof(m_playerstate); i++)
	{
		m_playerstate[i]=new StateStartup();
		m_changestate[i].m_newstate=-1;
		m_changestate[i].m_params=nullptr;
	}

	for(size_t i=0; i<countof(m_spriteoverlay); i++)
	{
		m_spriteoverlay[i].ticks=0;
	}

}

Game::~Game()
{

}

GameData &Game::GetGameData()
{
	return m_gamedata;
}

void Game::Update(const int ticks, const uint8_t nothing, Game *game)
{
	m_gamedata.m_ticks+=ticks;

	for(size_t i=0; i<countof(m_spriteoverlay); i++)
	{
		m_spriteoverlay[i].ticks-=ticks;
		if(m_spriteoverlay[i].ticks<0)
		{
			m_spriteoverlay[i].ticks=0;
		}
	}

	for(size_t i=0; i<countof(m_playerstate); i++)
	{
		m_playerstate[i]->Update(ticks,i,this);
	}

	HandleChangeState();

	// Handle AI for civ if it's their turn - also skip turn for dead civs
	if(m_gamedata.m_gamestarted==true && (m_gamedata.m_civplayernum[m_gamedata.m_currentcivturn]==0 || CivilizationAlive(m_gamedata.m_currentcivturn)==false))
	{
		if(m_gamedata.m_civplayernum[m_gamedata.m_currentcivturn]==0)
		{
			// TODO - handle AI turn
			trace("TODO - Handle AI");
		}
		EndPlayerTurn(m_gamedata.m_currentcivturn);
	}

	// check if player hasn't input in a while and set back to CPU for that civ
	for(size_t i=0; i<countof(m_playerstate); i++)
	{
		// 2 minute keyboard timeout (netplay only, and player not player 1)
		if(netplay_active() && i>0 && m_gamedata.m_playeractive[i]==true && m_gamedata.m_playerlastactivity[i]+(PLAYER_TIMEOUT)<m_gamedata.m_ticks)
		{
			// make sure to go to appropriate state
			m_gamedata.m_playeractive[i]=false;

			const int8_t ci=m_gamedata.GetCivIndexFromPlayerNum(i+1);
			if(ci>=0)
			{
				m_gamedata.ClearPlayerNumCivIndex(i+1);

				trace("timeout - changing state");
				StateMainMenuParams *mmp=new StateMainMenuParams(this);
				ChangeState(i,STATE_MAINMENU,mmp);
			}

		}
	}

	// check for turn timeout and end turn
	if(m_gamedata.m_gamestarted==true && TurnTicksLeft()==0)
	{
		EndPlayerTurn(m_gamedata.m_currentcivturn);
	}

}

uint64_t Game::GetTicks() const
{
	return m_gamedata.m_ticks;
}

void Game::Draw(const uint8_t nothing)
{

	m_playerstate[PlayerIndex()]->Draw(PlayerIndex());

}

bool Game::HandleInput(const Input *input, const uint8_t nothing)
{
	bool handled=false;

	for(size_t i=0; i<countof(m_playerstate); i++)
	{
		m_playerstate[i]->HandleInput(input,i);

		// check for any input and set last activity (use to time out)
		if(input->GamepadActivity(i+1))
		{
			m_gamedata.m_playerlastactivity[i]=m_gamedata.m_ticks;
			m_gamedata.m_playeractive[i]=true;
		}

	}

	return handled;
}

int8_t Game::PlayerIndex() const
{
	if(netplay_active()==true)
	{
		return netplay_playeridx();
	}
	return 0;
}

IState *Game::GetPlayerState(const uint8_t playerindex)
{
	if(playerindex>=0 && playerindex<countof(m_playerstate))
	{
		return m_playerstate[playerindex];
	}
	return 0;
}

uint8_t Game::PlayerCount() const
{
	return countof(m_playerstate);
}

void Game::ChangeState(const uint8_t playerindex, const uint8_t newstate, const IStateChangeParams *params)
{
	if(newstate>=0 && newstate<STATE_MAX && newstate!=m_playerstate[playerindex]->State() && playerindex>=0 && playerindex<countof(m_changestate))
	{
		// check if params were already set - meaning this state was already set to change and now change to a new state in the same update cycle
		if(m_changestate[playerindex].m_newstate>=0 || m_changestate[playerindex].m_params!=nullptr)
		{
			trace("Game::ChangeState params were already set!");
			delete m_changestate[playerindex].m_params;
		}
		m_changestate[playerindex].m_newstate=newstate;
		m_changestate[playerindex].m_params=(IStateChangeParams *)params;
	}
}

void Game::HandleChangeState()
{
	for(size_t i=0; i<countof(m_changestate); i++)
	{
		if(m_changestate[i].m_newstate>=0 && m_changestate[i].m_newstate!=m_playerstate[i]->State())
		{
			const uint8_t oldstate=m_playerstate[i]->State();
			delete m_playerstate[i];
			m_playerstate[i]=nullptr;

			switch(m_changestate[i].m_newstate)
			{
			case STATE_MAINMENU:
				m_playerstate[i]=new StateMainMenu();
				m_playerstate[i]->StateChanged(i,oldstate,m_changestate[i].m_params);
				break;
			case STATE_PREGAME:
				m_playerstate[i]=new StatePreGame();
				m_playerstate[i]->StateChanged(i,oldstate,m_changestate[i].m_params);
				break;
			case STATE_GAME:
				m_playerstate[i]=new StateGame(m_gamedata.m_map);
				m_playerstate[i]->StateChanged(i,oldstate,m_changestate[i].m_params);
				break;
			default:
				trace("Game::HandleChangeState State not impelemented!");
			}

			m_changestate[i].m_newstate=-1;
			if(m_changestate[i].m_params)
			{
				delete m_changestate[i].m_params;
				m_changestate[i].m_params=nullptr;
			}

		}
	}
}

int32_t Game::NextUnitIndex(const int8_t civindex, const int32_t currentunitindex) const
{
	const int32_t startindex=(currentunitindex>=0 && currentunitindex<countof(m_gamedata.m_unit) ? currentunitindex : 0);
	int32_t idx;
	for(int32_t i=0,idx=startindex+1; i<countof(m_gamedata.m_unit); i++,idx++)
	{
		if(idx>=countof(m_gamedata.m_unit))
		{
			idx=0;
		}
		if(m_gamedata.m_unit[idx].owner==civindex && (m_gamedata.m_unit[idx].flags & UNIT_ALIVE)==UNIT_ALIVE)
		{
			return idx;
		}
	}
	return -1;
}

int32_t Game::NextCityIndex(const int8_t civindex, const int32_t currentcityindex) const
{
	const int32_t startindex=(currentcityindex>=0 && currentcityindex<countof(m_gamedata.m_city) ? currentcityindex : 0);
	int32_t idx;
	for(int32_t i=0,idx=startindex+1; i<countof(m_gamedata.m_city); i++,idx++)
	{
		if(idx>=countof(m_gamedata.m_city))
		{
			idx=0;
		}
		if(m_gamedata.m_city[idx].owner==civindex && (m_gamedata.m_city[idx].population>0))
		{
			return idx;
		}
	}
	return -1;
}

int32_t Game::UnitIndexAtLocation(const int8_t owneridx, const int32_t x, const int32_t y) const
{
	const MapCoord mc(m_gamedata.m_map->Width(),m_gamedata.m_map->Height(),x,y);
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if(((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE) && m_gamedata.m_unit[i].x==mc.X() && m_gamedata.m_unit[i].y==mc.Y() && (owneridx==-1 || owneridx==m_gamedata.m_unit[i].owner))
		{
			return i;
		}
	}
	return -1;
}

int32_t Game::CityIndexAtLocation(const int32_t x, const int32_t y) const
{
	const MapCoord mc(m_gamedata.m_map->Width(),m_gamedata.m_map->Height(),x,y);
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0 && m_gamedata.m_city[i].x==mc.X() && m_gamedata.m_city[i].y==mc.Y())
		{
			return i;
		}
	}
	return -1;
}

int32_t Game::CityFoodStorage(const int32_t cityindex) const
{
	int32_t storage=0;
	if(cityindex>=0 && cityindex<countof(m_gamedata.m_city))
	{
		storage=m_gamedata.m_city[cityindex].population*((m_gamedata.m_city[cityindex].improvements & (0x01 << IMPROVEMENT_GRANARY)) == (0x01 << IMPROVEMENT_GRANARY) ? 50 : 25);
	}
	return storage;
}

int32_t Game::CityGrowthFoodRequired(const int32_t cityindex) const
{
	int32_t food=0;
	if(cityindex>=0 && cityindex<countof(m_gamedata.m_city))
	{
		food=m_gamedata.m_city[cityindex].population*25;
	}
	return food;
}

int8_t Game::PlayerCivIndex(const int8_t playerindex) const
{
	return m_gamedata.GetCivIndexFromPlayerNum(playerindex+1);
}

bool Game::IsPlayerTurn(const uint8_t playerindex) const
{
	return m_gamedata.m_currentcivturn==PlayerCivIndex(playerindex);
}

int64_t Game::TurnTicksLeft() const
{
	if(m_gamedata.m_turnstarttick+(uint64_t)m_gamedata.m_turntimelimit>m_gamedata.m_ticks)
	{
		return (uint64_t)m_gamedata.m_turntimelimit-(m_gamedata.m_ticks-m_gamedata.m_turnstarttick);
	}
	return 0;
}

void Game::EndPlayerTurn(const uint8_t playerindex)
{
	if(CivilizationAlive(m_gamedata.m_currentcivturn)==false)
	{
		m_gamedata.m_civ[m_gamedata.m_currentcivturn].gold=0;
	}

	m_gamedata.m_currentcivturn++;
	m_gamedata.m_turnstarttick=m_gamedata.m_ticks;

	// last civ for this game turn
	if(m_gamedata.m_currentcivturn==countof(m_gamedata.m_civ))
	{
		EndGameTurn();
	}

	// Update logic will take care of AI players when it's their turn
}

void Game::EndGameTurn()
{
	// TODO - adjust resources for all civs at end of game turn
	m_gamedata.m_currentcivturn=0;
	m_gamedata.m_gameturn++;

	// recalculate the turn time limit for all players (they all get the same timee limit for each game turn)
	m_gamedata.m_turntimelimit=(4*60*60);	// 4 minutes
	// + 10 seconds for each city with pop
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0)
		{
			m_gamedata.m_turntimelimit+=(10*60);
		}
	}

	// TODO - adjust movement points for all units
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE)==UNIT_ALIVE)
		{
			m_gamedata.m_unit[i].movesleft=unitdata[m_gamedata.m_unit[i].type].moves;
		}

		// TODO - make sure enough food/resources for units and remove if not
		// TODO - make sure land unit on water is embarked
		// TODO - make sure small water unit stopped in shallow water
	}
	
	// 1st add produced food/resources to cities, and gold to their civ - we'll check city and unit upkeep later
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0)
		{
			CityProduction cprod=GetCityProduction(i);
			m_gamedata.m_city[i].shields+=cprod.totalresources;
			m_gamedata.m_city[i].food+=cprod.totalfood;
			m_gamedata.m_civ[m_gamedata.m_city[i].owner].gold+=cprod.totalgold;
		}
	}
	
	// make sure units have enough resouces from their home city - if not then disband them
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE)
		{
			const int32_t ci=i/UNITS_PER_CITY;
			// not enough resources - disband unit
			if(m_gamedata.m_city[ci].shields<unitdata[m_gamedata.m_unit[i].type].consumeresources || m_gamedata.m_city[ci].food<unitdata[m_gamedata.m_unit[i].type].consumefood || m_gamedata.m_civ[m_gamedata.m_unit[i].owner].gold<unitdata[m_gamedata.m_unit[i].type].consumegold)
			{
				DisbandUnit(m_gamedata.m_unit[i].owner,i,false);
			}
			// enough resources - take away from city/civ
			else
			{
				m_gamedata.m_city[ci].shields-=unitdata[m_gamedata.m_unit[i].type].consumeresources;
				m_gamedata.m_city[ci].food-=unitdata[m_gamedata.m_unit[i].type].consumefood;
				m_gamedata.m_civ[m_gamedata.m_unit[i].owner].gold-=unitdata[m_gamedata.m_unit[i].type].consumegold;
			}
		}
	}

	// handle upkeep for improvements, if there's not enough gold, sell improvement (receive 1/5 price of buying improvement) (unit upkeep gold was already removed above)
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0)
		{
			for(int32_t ii=15; ii>=0; ii--)	// start at the back (so better improvements get sold first)
			{
				if((m_gamedata.m_city[i].improvements & (0x01 << ii)) == (0x01 << ii))
				{
					// enough gold for improvement upkeep
					if(m_gamedata.m_civ[m_gamedata.m_city[i].owner].gold>=improvementdata[ii].upkeepgold)
					{
						m_gamedata.m_civ[m_gamedata.m_city[i].owner].gold-=improvementdata[ii].upkeepgold;
					}
					// not enough gold for improvement upkeep - sell improvement and add gold to civ
					else
					{
						m_gamedata.m_city[i].improvements=m_gamedata.m_city[i].improvements & (~(0x01 << ii));
						m_gamedata.m_civ[m_gamedata.m_city[i].owner].gold+=(improvementdata[ii].sellgold);
					}
				}
			}
		}
	}

	// now adjust city population based on stored food and adjust civ gold for improvement upkeep 
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0)
		{
			// enough food for population
			if(m_gamedata.m_city[i].food>=m_gamedata.m_city[i].population)
			{
				m_gamedata.m_city[i].food-=m_gamedata.m_city[i].population;
			}
			// not enough food for population - reduce population
			else
			{
				m_gamedata.m_city[i].population--;
				m_gamedata.m_city[i].food=CityGrowthFoodRequired(i)/2;
			}

			// check if we have enough food to expand city
			if(m_gamedata.m_city[i].food>=CityGrowthFoodRequired(i))
			{
				if(CityCanExpand(i)==true)
				{
					m_gamedata.m_city[i].population++;
					m_gamedata.m_city[i].food=CityGrowthFoodRequired(i)/2;
				}
			}

			// make sure we don't have more food than storage limit
			if(m_gamedata.m_city[i].food>CityFoodStorage(i))
			{
				m_gamedata.m_city[i].food=CityFoodStorage(i);
			}
		}
	}

	// TODO - if shield production matches what's needed for building unit/improvement then complete it - set building back to none if it was improvement, otherwise leave at unit
	// if no more unit slots available, then keep -1 resource than needed so the building can be changed in city detail view
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0)
		{
			if(m_gamedata.m_city[i].producing!=0)
			{
				if(buildingxref[m_gamedata.m_city[i].producing].buildingtype==BUILDINGTYPE_UNIT)
				{
					const int32_t udi=buildingxref[m_gamedata.m_city[i].producing].building;
					if(m_gamedata.m_city[i].shields>=unitdata[udi].buildresources)
					{
						int32_t ui=FreeUnitIndex(i);
						if(ui>=0)
						{
							m_gamedata.m_city[i].shields-=unitdata[udi].buildresources;
							m_gamedata.m_unit[ui].flags=UNIT_ALIVE;
							// if barracked then set veteran
							if((m_gamedata.m_city[i].improvements & (0x01 << IMPROVEMENT_BARRACKS))==(0x01 << IMPROVEMENT_BARRACKS))
							{
								m_gamedata.m_unit[ui].flags|=UNIT_VETERAN;
							}
							m_gamedata.m_unit[ui].type=udi;
							m_gamedata.m_unit[ui].owner=m_gamedata.m_city[i].owner;
							m_gamedata.m_unit[ui].x=m_gamedata.m_city[i].x;
							m_gamedata.m_unit[ui].y=m_gamedata.m_city[i].y;
							m_gamedata.m_unit[ui].movesleft=unitdata[udi].moves;

							// creating settler reduces city pop
							if(udi==UNITTYPE_SETTLER)
							{
								m_gamedata.m_city[i].population--;
							}

							// leave same unit being produced until user changes it

						}
						// no free unit index - set city resource to -1 than needed
						else
						{
							m_gamedata.m_city[i].shields=unitdata[m_gamedata.m_city[i].producing].buildresources-1;
						}
					}
				}
				else if(buildingxref[m_gamedata.m_city[i].producing].buildingtype==BUILDINGTYPE_IMPROVEMENT)
				{
					if(m_gamedata.m_city[i].shields>=improvementdata[buildingxref[m_gamedata.m_city[i].producing].building].buildresources)
					{
						m_gamedata.m_city[i].improvements |= (0x01 << buildingxref[m_gamedata.m_city[i].producing].building);
						m_gamedata.m_city[i].producing=BUILDING_NONE;
						//m_gamedata.m_city[i].shields-=improvementdata[m_gamedata.m_city[i].producing].buildresources;
						m_gamedata.m_city[i].shields=0;
					}
				}
				// TODO - future other building types??
			}
		}
	}

	// TODO - go through cities - make sure production enough to handle units - if not then remove units until it is
	// expand food storage and when hit max expand population
	// add and built units
	// make sure total gold is enough and sell improvements if it isn't
	// if unit building and no space for unit - leave -1 resource in so they can change build

	// save game after every turn
	m_gamedata.SaveGame();
}

int32_t Game::CityInRadius(const int8_t owneridx, const int32_t x, const int32_t y, const int32_t radius) const
{
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0 && (owneridx==-1 || m_gamedata.m_city[i].owner==owneridx))
		{
			int32_t dx=(int32_t)m_gamedata.m_city[i].x-x;
			int32_t dy=(int32_t)m_gamedata.m_city[i].y-y;
			dx=dx<0 ? -dx : dx;
			dy=dy<0 ? -dy : dy;
			if(dx<=radius && dy<=radius)
			{
				return i;
			}
		}
	}
	return -1;
}

int32_t Game::FreeCityIndex(const int8_t owneridx) const
{
	if(owneridx>=0 && owneridx<countof(m_gamedata.m_civ))
	{
		for(size_t i=(owneridx*CITIES_PER_CIVILIZATION); i<(owneridx*CITIES_PER_CIVILIZATION)+CITIES_PER_CIVILIZATION; i++)
		{
			if(m_gamedata.m_city[i].population==0)
			{
				return i;
			}
		}
	}
	return -1;
}

int32_t Game::FreeUnitIndex(const int32_t cityindex) const
{
	if(cityindex>=0 && cityindex<countof(m_gamedata.m_city))
	{
		for(size_t i=(cityindex*UNITS_PER_CITY); i<(cityindex*UNITS_PER_CITY)+UNITS_PER_CITY; i++)
		{
			if((m_gamedata.m_unit[i].flags & UNIT_ALIVE)!=UNIT_ALIVE)
			{
				return i;
			}
		}
	}
	return -1;
}

bool Game::CanFoundCity(const uint8_t playerindex, const int32_t settlerindex) const
{
	// unit type must be settler, we must be on land, no other city must be within 3 spaces radius, and we must have a free city slot for this civ
	const int8_t civi=PlayerCivIndex(playerindex);
	if(civi>=0 && civi<countof(m_gamedata.m_civ) && settlerindex>=0 && settlerindex<countof(m_gamedata.m_unit))
	{
		const Unit *u=&(m_gamedata.m_unit[settlerindex]);
		if((u->flags & UNIT_ALIVE)==UNIT_ALIVE && u->type==UNITTYPE_SETTLER && u->owner==civi && u->movesleft>0)
		{
			if(m_gamedata.m_map->GetBaseType(u->x,u->y)==BaseTerrain::BASETERRAIN_LAND && CityInRadius(-1,u->x,u->y,4)==-1)
			{
				if(FreeCityIndex(civi)>-1)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool Game::FoundCity(const uint8_t playerindex, const int32_t settlerindex)
{
	if(CanFoundCity(playerindex,settlerindex)==true)
	{
		const int8_t civi=PlayerCivIndex(playerindex);
		m_gamedata.m_unit[settlerindex].flags=0;

		int8_t ci=FreeCityIndex(civi);
		memset(&(m_gamedata.m_city[ci]),0,sizeof(City));
		m_gamedata.m_city[ci].x=m_gamedata.m_unit[settlerindex].x;
		m_gamedata.m_city[ci].y=m_gamedata.m_unit[settlerindex].y;
		m_gamedata.m_city[ci].owner=civi;
		m_gamedata.m_city[ci].population=1;

		return true;
	}
	return false;
}

bool Game::ExpandCity(const uint8_t playerindex, const int32_t settlerindex)
{
	if(playerindex>=0 && playerindex<countof(m_gamedata.m_civ) && settlerindex>=0 && settlerindex<countof(m_gamedata.m_unit))
	{
		Unit *u=&(m_gamedata.m_unit[settlerindex]);
		if((u->flags & UNIT_ALIVE)==UNIT_ALIVE && u->type==UNITTYPE_SETTLER && u->owner==PlayerCivIndex(playerindex))
		{
			int32_t ci=CityIndexAtLocation(u->x,u->y);
			if(ci>=0)
			{
				m_gamedata.m_city[ci].population++;
				u->flags=0;
				return true;
			}
		}
	}

	return false;
}

bool Game::CityCanExpand(const int32_t cityidx) const
{
	if(cityidx>=0 && cityidx<countof(m_gamedata.m_city))
	{
		// ok to expand if population <8 or population<15 and have granary or population<25 and have aqueduct and granary
		if(m_gamedata.m_city[cityidx].population>0 && 
			(
			(m_gamedata.m_city[cityidx].population<8)
			||
			(m_gamedata.m_city[cityidx].population<15 && (m_gamedata.m_city[cityidx].improvements & (0x01 << IMPROVEMENT_GRANARY)) == (0x01 << IMPROVEMENT_GRANARY))
			||
			(m_gamedata.m_city[cityidx].population<25 && (m_gamedata.m_city[cityidx].improvements & (0x01 << IMPROVEMENT_GRANARY)) == (0x01 << IMPROVEMENT_GRANARY) && (m_gamedata.m_city[cityidx].improvements & (0x01 << IMPROVEMENT_AQUEDUCT)) == (0x01 << IMPROVEMENT_AQUEDUCT))
			)
		)
		{
			return true;
		}

	}
	return false;
}

bool Game::DisbandUnit(const int8_t playerindex, const int32_t unitindex, const bool killed)
{
	if(playerindex==-1 || (playerindex>=0 && playerindex<countof(m_gamedata.m_civ)) && unitindex>=0 && unitindex<countof(m_gamedata.m_unit))
	{
		Unit *u=&(m_gamedata.m_unit[unitindex]);
		if(u->owner==PlayerCivIndex(playerindex) || playerindex==-1)
		{
			// if water unit on sea then disband all embarked units
			TerrainTile tt=m_gamedata.m_map->GetTile(u->x,u->y);
			if(tt.BaseType()==BaseTerrain::BASETERRAIN_WATER && (unitdata[u->type].flags & UNITDATA_MOVE_WATER) == UNITDATA_MOVE_WATER)
			{
				for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
				{
					if(i!=unitindex && UnitEmbarkedShipIndex(i)==unitindex)
					{
						m_gamedata.m_unit[i].flags=0;
					}
				}
			}

			// TODO - if we disband in a city without being killed, should we add gold??

			u->flags=0;
			return true;
		}
	}
	return false;
}

int32_t Game::UnitCountAtLocation(const uint32_t x, const int32_t y) const
{
	int32_t count=0;
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[i].x==x && m_gamedata.m_unit[i].y==y)
		{
			count++;
		}
	}
	return count;
}

SpriteSheetPos Game::GetCitySpriteSheetPos(const int32_t cityidx) const
{
	if(cityidx>=0 && cityidx<countof(m_gamedata.m_city) && m_gamedata.m_city[cityidx].population>0)
	{
		// sprite idx start position
		int32_t sxidx=0+((m_gamedata.m_city[cityidx].owner%2)*4);
		int32_t syidx=7+(m_gamedata.m_city[cityidx].owner/2);

		if(m_gamedata.m_city[cityidx].population>4)
		{
			sxidx++;
		}
		if(m_gamedata.m_city[cityidx].population>8)
		{
			sxidx++;
		}
		if(m_gamedata.m_city[cityidx].population>12)
		{
			sxidx++;
		}

		// sprite idx
		return SpriteSheetPos(sxidx,syidx);
	}
	return SpriteSheetPos();
}

CityProduction Game::GetCityProduction(const int32_t cityidx) const
{
	CityProduction prod;
	memset(&prod,0,sizeof(CityProduction));

	if(cityidx>=0 && cityidx<countof(m_gamedata.m_city) && m_gamedata.m_city[cityidx].population>0)
	{
		const City *c=&(m_gamedata.m_city[cityidx]);

		// 1st get upkeep of improvements, then upkeep of any units
		prod.unitupkeepfood=0;
		prod.unitupkeepresources=0;
		prod.unitupkeepgold=0;
		prod.totalupkeepfood=c->population;		// population requires 1 food each
		prod.totalupkeepresources=0;
		prod.totalupkeepgold=0;

		for(size_t i=0; i<16; i++)
		{
			if((c->improvements & (0x01 << i)) == (0x01 << i))
			{
				prod.totalupkeepgold+=improvementdata[i].upkeepgold;	// improvements only require gold for upkeep
			}
		}
		for(size_t i=(cityidx*UNITS_PER_CITY); i<(cityidx*UNITS_PER_CITY)+UNITS_PER_CITY; i++)
		{
			if((m_gamedata.m_unit[i].flags & UNIT_ALIVE)==UNIT_ALIVE && m_gamedata.m_unit[i].owner==c->owner)
			{
				prod.unitupkeepfood+=unitdata[m_gamedata.m_unit[i].type].consumefood;
				prod.unitupkeepresources+=unitdata[m_gamedata.m_unit[i].type].consumeresources;
				prod.unitupkeepgold+=unitdata[m_gamedata.m_unit[i].type].consumegold;
				prod.totalupkeepfood+=unitdata[m_gamedata.m_unit[i].type].consumefood;
				prod.totalupkeepresources+=unitdata[m_gamedata.m_unit[i].type].consumeresources;
				prod.totalupkeepgold+=unitdata[m_gamedata.m_unit[i].type].consumegold;
			}
		}

		// now get resource production
		MapCoord mc(m_gamedata.m_map->Width(),m_gamedata.m_map->Height(),0,0);

		// 1st get possible resource production of every tile in city
		int32_t idx=0;
		for(int32_t dy=-2; dy<3; dy++)
		{
			for(int32_t dx=-2; dx<3; dx++,idx++)
			{
				mc.Set(dx+((int32_t)c->x),dy+((int32_t)c->y));
				TerrainTile tt=m_gamedata.m_map->GetTile(mc.X(),mc.Y());
				prod.tile[idx].food+=TerrainTile::terrainproduction[tt.Type()].food+TerrainTile::resourceproduction[tt.Resource()].food;
				prod.tile[idx].resources+=TerrainTile::terrainproduction[tt.Type()].resources+TerrainTile::resourceproduction[tt.Resource()].resources;
			
				// if there's an enemy on this tile - then we don't get any production from it
				if(UnitIndexAtLocation(-1,mc.X(),mc.Y())>=0)
				{
					if(m_gamedata.m_unit[UnitIndexAtLocation(-1,mc.X(),mc.Y())].owner!=c->owner)
					{
						prod.tile[idx].food=0;
						prod.tile[idx].resources=0;
					}
				}
			
			}
		}

		// city center always starts with +1 food and +1 resource no matter underlying terrain
		prod.tile[12].food+=1;
		prod.tile[12].resources+=1;

		// tile 12 will always be included (city center), so it starts at the front of the sort
		int8_t sortorder[25]={12,1,2,3,4,5,6,7,8,9,10,11,13,14,15,16,17,18,19,20,21,22,23,24};
		for(int8_t i=1; i<24; i++)
		{
			for(int8_t j=1; j<25-i; j++)
			{
				// favor food tiles unless food+resources are at least +2 in the other tile
				if(
					((prod.tile[sortorder[j]].food+prod.tile[sortorder[j]].resources+1) < (prod.tile[sortorder[j+1]].food+prod.tile[sortorder[j+1]].resources))
					||
					(prod.tile[sortorder[j]].food < prod.tile[sortorder[j+1]].food) 
					|| 
					(prod.tile[sortorder[j]].food == prod.tile[sortorder[j+1]].food && prod.tile[sortorder[j]].resources < prod.tile[sortorder[j+1]].resources)
					)
				{
					int8_t temp=sortorder[j];
					sortorder[j]=sortorder[j+1];
					sortorder[j+1]=temp;
				}
			}
		}

		// clear out prod for tiles >population
		for(int8_t i=c->population; i<25; i++)
		{
			prod.tile[sortorder[i]].food=0;
			prod.tile[sortorder[i]].resources=0;
		}

		// get total
		for(int8_t i=0; i<25; i++)
		{
			prod.totalfood+=prod.tile[i].food;
			prod.totalresources+=prod.tile[i].resources;
		}

		// if city has factor then 50% more resources;
		float mult=1.0;
		if(c->improvements & (0x01 << IMPROVEMENT_FACTORY))
		{
			mult+=0.5;
		}
		prod.totalresources=((float)prod.totalresources*mult)+0.5;

		// if city has market then 50% more food;
		mult=1.0;
		if(c->improvements & (0x01 << IMPROVEMENT_MARKET))
		{
			mult+=0.5;
		}
		prod.totalfood=((float)prod.totalfood*mult)+0.5;		// +0.5 to round result
		
		// if city has bank then 50% more gold
		mult=1.0;
		if(c->improvements & (0x01 << IMPROVEMENT_BANK))
		{
			mult+=0.5;
		}
		// food taxed at 1x rate, resources taxed at 10x.  If we're building none, then half of all extra resources (minus upkeep resources) get converted to gold
		int32_t surplusresources=prod.totalresources-prod.totalupkeepresources;
		prod.totalgold=(((TAX_RATE*(float)prod.totalfood)+(TAX_RATE*(float)prod.totalresources*10.0)+((c->producing==0 && surplusresources>0) ? ((float)surplusresources)*0.5 : 0.0f))*mult)+0.5;		// +0.5 to round result
		if(c->producing==0)
		{
			if(prod.totalupkeepresources<=prod.totalresources)
			{
				prod.totalresources=prod.totalupkeepresources;
			}
			// otherwise we don't have enough total resource production to support all units and some unit will have to be disbanded end of turn
		}
	}

	return prod;
}

int32_t Game::ShipAtLocation(const int32_t x, const int32_t y) const
{
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE && (unitdata[m_gamedata.m_unit[i].type].flags & UNITDATA_MOVE_WATER) == UNITDATA_MOVE_WATER && m_gamedata.m_unit[i].x==x && m_gamedata.m_unit[i].y==y)
		{
			return i;
		}
	}
	return -1;
}

bool Game::EmbarkableShipAtLocation(const uint8_t playerindex, const int32_t x, const int32_t y) const
{
	const int32_t si=ShipAtLocation(x,y);
	if(si>=0)
	{
		const Unit *u=&(m_gamedata.m_unit[si]);
		if(u->owner==PlayerCivIndex(playerindex))
		{
			uint8_t embarkcount=0;
			// get count of non water units at location (they must be embarked on this ship)
			for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
			{
				if(si!=i && m_gamedata.m_unit[i].x==u->x && m_gamedata.m_unit[i].y==u->y)
				{
					embarkcount++;
				}
			}
			// room left to embark a new unit
			if(embarkcount<unitdata[u->type].transport)
			{
				return true;
			}
		}
	}
	return false;
}

int32_t Game::EnemyShipAtLocation(const uint8_t playerindex, const int32_t x, const int32_t y) const
{
	const int32_t si=ShipAtLocation(x,y);
	return (si>=0 && m_gamedata.m_unit[si].owner!=PlayerCivIndex(playerindex) ? si : -1);
}

int32_t Game::UnitEmbarkedShipIndex(const int32_t unitindex) const
{
	if(unitindex>=0 && unitindex<countof(m_gamedata.m_unit) && (m_gamedata.m_unit[unitindex].flags & UNIT_ALIVE) == UNIT_ALIVE)
	{
		const TerrainTile tt=m_gamedata.m_map->GetTile(m_gamedata.m_unit[unitindex].x,m_gamedata.m_unit[unitindex].y);

		// if tile unit is on is water, and unit can't move on water, then it has to be embarked on a ship
		if(tt.BaseType()==BaseTerrain::BASETERRAIN_WATER && (unitdata[m_gamedata.m_unit[unitindex].type].flags & UNITDATA_MOVE_WATER) != UNITDATA_MOVE_WATER)
		{
			// check for the ship it's emarked on
			for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
			{
				if(i!=unitindex && (m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[i].owner==m_gamedata.m_unit[unitindex].owner && (unitdata[m_gamedata.m_unit[i].type].flags & UNITDATA_MOVE_WATER)==UNITDATA_MOVE_WATER && m_gamedata.m_unit[i].x==m_gamedata.m_unit[unitindex].x && m_gamedata.m_unit[i].y==m_gamedata.m_unit[unitindex].y && unitdata[m_gamedata.m_unit[i].type].transport>0)
				{
					return i;
				}
			}
		}
	}
	return -1;
}

// TODO - implement move unit here
bool Game::MoveUnit(const uint8_t playerindex, const int32_t unitindex, const int32_t dx, const int32_t dy)
{
	// make sure player turn
	// make sure unit alive
	// make sure moving to terrain unit can move onto
	// make sure unit is not embarked if on ship
	// make sure move all embarked units if ship
	// make sure no enemy or enemy city on destination - if so then attack


	/* 2023-11-04 - TODO
		If land unit is on land moving to land without any enemy city or unit, then move
		If land unit is on land moving to water with friendly embarkable ship with space, then move and set movesleft to 0
		If land unit is on land moving to water with enemy ship then attack only
		If land unit is on land moving to enemy unit only then attack and move
		If land unit is on land moving to enemy city with enemy unit then attack only
		If land unit is on land moving to enemy city with no defenders then capture city and move
		If land unit is embarked, and moving to land without any enemy city or unit, then move and set movesleft to 0

	*/


	// land unit -> no enemy land - move OK (doesn't matter if starting from land or water) - starting from water just set moves left to 0 after move
	// water unit -> no enemy water - move OK





	if(IsPlayerTurn(playerindex)==true && unitindex>=0 && unitindex<countof(m_gamedata.m_unit) && m_gamedata.m_unit[unitindex].owner==PlayerCivIndex(playerindex) && (m_gamedata.m_unit[unitindex].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[unitindex].movesleft>0)
	{
		// TODO - land to sea - embarkable ship at destination is good to move - enemy ship at location then attack
		// TODO - sea unit - only 1 per tile
		RandomMT rand;
		rand.Seed(m_gamedata.m_ticks);
		Unit *u=&(m_gamedata.m_unit[unitindex]);
		MapCoord mc(m_gamedata.m_map->Width(),m_gamedata.m_map->Height(),((int32_t)u->x)+dx,((int32_t)u->y)+dy);
		const TerrainTile desttile=m_gamedata.m_map->GetTile(mc.X(),mc.Y());
		const TerrainTile sourcetile=m_gamedata.m_map->GetTile(u->x,u->y);
		// TODO - need to change this if air units are added
		const BaseTerrain::TerrainType unitterrain=(unitdata[u->type].flags & UNITDATA_MOVE_LAND)==UNITDATA_MOVE_LAND ? BaseTerrain::BASETERRAIN_LAND : BaseTerrain::BASETERRAIN_WATER;

		// check for enemy unit at destination
		Unit *eu=nullptr;
		int32_t eucount=0;	// enemy unit count at location
		for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
		{
			if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[i].x==mc.X() && m_gamedata.m_unit[i].y==mc.Y() && m_gamedata.m_unit[i].owner!=u->owner)
			{
				eu=&(m_gamedata.m_unit[i]);
				eucount++;
			}
		}
		// if dest tile is water, then the enemy unit found above may not be the ship but an embarked unit, so we need to find the ship to attack
		if(desttile.BaseType()==BaseTerrain::BASETERRAIN_WATER && EnemyShipAtLocation(playerindex,mc.X(),mc.Y())>=0)
		{
			eu=&(m_gamedata.m_unit[EnemyShipAtLocation(playerindex,mc.X(),mc.Y())]);
			eucount=1;	// only ship counts because when it's destoryed all the embarked units are destroyed
		}
		// check for enemy city at destination
		City *ec=nullptr;
		int32_t eci=CityIndexAtLocation(mc.X(),mc.Y());
		if(eci>=0 && m_gamedata.m_city[eci].owner!=PlayerCivIndex(playerindex))
		{
			ec=&(m_gamedata.m_city[eci]);
		}

		bool trymove=true;
		bool attackok=false;

		//debug
		OutputStringStream ostr;
		ostr << "(" << u->x << "," << u->y << ") - (" << mc.X() << "," << mc.Y() << ") eu=" << int32_t(eu) << " ec=" << int32_t(ec) << " ecnt=" << eucount;

		// land unit -> must be disembarked to attack - land unit/city or ship on water - attack ok
		if(unitterrain==BaseTerrain::BASETERRAIN_LAND && (eu || ec) && UnitEmbarkedShipIndex(unitindex)<0)
		{
			attackok=true;
		}
		// water unit -> can only attack land unit or water unit - no empty cities
		else if(unitterrain==BaseTerrain::BASETERRAIN_WATER && eu)
		{
			attackok=true;
		}

		ostr << " aok=" << (int32_t)attackok;

		if(attackok)
		{
			float defmult=1.0;
			
			// if enemy city, but no units, then we take the city
			if(ec && !eu)
			{
				ec->owner=PlayerCivIndex(playerindex);
				// clear out enemy units with this home city
				for(size_t i=(eci*UNITS_PER_CITY); i<(eci*UNITS_PER_CITY)+UNITS_PER_CITY; i++)
				{
					m_gamedata.m_unit[i].flags=0;
				}
				trymove=true;
				// enemy city is no longer an enemy city so we clear it out - this allows us to move in on the same turn we capture
				ec=nullptr;
			}
			else if(eu)
			{
				// enemy unit in an enemy city - gets bonus to defense and bonus if city wall
				if(ec)
				{
					defmult+=0.5;
					if((ec->improvements & (0x01 << IMPROVEMENT_CITYWALLS)) == (0x01 << IMPROVEMENT_CITYWALLS))
					{
						defmult+=0.5;
					}
				}

				float defvet=((eu->flags & UNIT_VETERAN) == UNIT_VETERAN) ? 1.0 : 0.0;
				float attvet=((u->flags & UNIT_VETERAN) == UNIT_VETERAN) ? 1.0 : 0.0;
				float def=((float)unitdata[eu->type].defense+defvet)*defmult*rand.NextDouble();
				float att=((float)unitdata[u->type].attack+attvet)*rand.NextDouble();
				// attacker died
				if(def>att)
				{
					trymove=false;
					AddSpriteOverlay(u->x,u->y,SpriteSheetPos(2,0),60);
					DisbandUnit(-1,unitindex,true);
					eu->flags|=UNIT_VETERAN;

					ostr << " att died uf=" << u->flags;
				}
				// defender died (there may be other defender units still there)
				else
				{
					trymove=true;
					u->flags|=UNIT_VETERAN;
					AddSpriteOverlay(eu->x,eu->y,SpriteSheetPos(2,0),60);
					DisbandUnit(-1,eu-m_gamedata.m_unit,true);

					// if the enemy was in a city - then we killed the unit, take a movement point away and don't try to enter city
					// if there was more than 1 enemy at dest, take a movement point away and don't try to move
					if(ec || eucount>1)
					{
						u->movesleft--;
						if(desttile.Type()==TerrainTile::TERRAIN_MOUNTAIN)
						{
							u->movesleft=0;
						}
						trymove=false;

						// chance to reduce population or destory improvement
						// if our attack 1 or more greater than defense, then chance to destroy improvement
						// if our attack was 2 or more greater than defense, then chance to reduce population
						if(ec && ec->population>0)
						{
							if(att>def+1.0 && rand.NextDouble()<0.1)
							{
								// now get a random improvement (city may or may not have the improvement, so less chance improvements are destoryed the fewer there are)
								const int32_t ii=rand.NextDouble()*IMPROVEMENT_MAX;
								ec->improvements=ec->improvements & ~(0x01 << ii);
							}
							if(att>def+2.0 && rand.NextDouble()<0.1)
							{
								ec->population--;
							}
						}

					}

					ostr << " def died df=" << eu->flags;

				}
			}
		}

		ostr << " tmove=" << trymove << " ml=" << u->movesleft;
		trace(ostr.Buffer());

		if(trymove==true && u->movesleft>0 && 
			(
			// land unit can move to land (without any enemy unit or city)
			(unitterrain==BaseTerrain::BASETERRAIN_LAND && desttile.BaseType()==BaseTerrain::BASETERRAIN_LAND && !eu && !ec) 
			|| 
			// land unit can move on embarkable ship
			(unitterrain==BaseTerrain::BASETERRAIN_LAND && desttile.BaseType()==BaseTerrain::BASETERRAIN_WATER && EmbarkableShipAtLocation(playerindex,mc.X(),mc.Y())==true)
			||
			// water unit can move to empty water OR move to land with friendly city (not enemy city)
			(unitterrain==BaseTerrain::BASETERRAIN_WATER && ((desttile.BaseType()==BaseTerrain::BASETERRAIN_WATER && ShipAtLocation(mc.X(),mc.Y())<0) || (!ec && CityIndexAtLocation(mc.X(),mc.Y())>=0)))
			)
		)
		{
			// ship starting on water moves embarked units first (in case we just left port to sea we don't take units in city with ship)
			if(unitterrain==BaseTerrain::BASETERRAIN_WATER && sourcetile.BaseType()==BaseTerrain::BASETERRAIN_WATER)
			{
				uint8_t tc=0;
				for(size_t i=0; i<countof(m_gamedata.m_unit) && tc<unitdata[u->type].transport; i++)
				{
					if(i!=unitindex && ((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE) && m_gamedata.m_unit[i].owner==u->owner && m_gamedata.m_unit[i].x==u->x && m_gamedata.m_unit[i].y==u->y)
					{
						m_gamedata.m_unit[i].x=mc.X();
						m_gamedata.m_unit[i].y=mc.Y();
					}
				}
			}

			u->x=mc.X();
			u->y=mc.Y();

			

			// subtract move
			u->movesleft--;
			if(desttile.Type()==TerrainTile::TERRAIN_MOUNTAIN)
			{
				u->movesleft=0;
			}
			// land unit embarked/disembarked - 0 moves left (check land-water or water-land)
			if(unitterrain==BaseTerrain::BASETERRAIN_WATER && sourcetile.BaseType()!=desttile.BaseType())
			{
				u->movesleft=0;
			}

			return true;
		}
	}

	return false;
}

void Game::AddSpriteOverlay(const int32_t mapx, const int32_t mapy, const SpriteSheetPos spos, const int64_t ticks)
{
	for(size_t i=0; i<countof(m_spriteoverlay); i++)
	{
		if(m_spriteoverlay[i].ticks==0)
		{
			m_spriteoverlay[i].mapx=mapx;
			m_spriteoverlay[i].mapy=mapy;
			m_spriteoverlay[i].sprite=spos;
			m_spriteoverlay[i].ticks=ticks;
			return;
		}
	}
}

void Game::DrawSpriteOverlay(const int32_t mapx, const int32_t mapy, const int32_t screenx, const int32_t screeny)
{
	for(size_t i=0; i<countof(m_spriteoverlay); i++)
	{
		if(m_spriteoverlay[i].ticks>0 && m_spriteoverlay[i].mapx==mapx && m_spriteoverlay[i].mapy==mapy)
		{
			*DRAW_COLORS=(PALETTE_WHITE << 4) | PALETTE_BLACK;
			blitMasked(icongfx,icongfxalpha,screenx,screeny,16,16,m_spriteoverlay[i].sprite.m_xidx*16,m_spriteoverlay[i].sprite.m_yidx*16,icongfxwidth,BLIT_1BPP);
		}
	}
}

bool Game::CivilizationAlive(const uint8_t civindex) const
{
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0 && m_gamedata.m_city[i].owner==civindex)
		{
			return true;
		}
	}
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[i].owner==civindex)
		{
			return true;
		}
	}

	return false;
}
