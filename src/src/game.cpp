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
			HandleAI(m_gamedata.m_currentcivturn);
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

				//trace("timeout - changing state");
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
			//trace("Game::ChangeState params were already set!");
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
				//trace("Game::HandleChangeState State not impelemented!");
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
	return NextUnitAtLocIndex(civindex,-999,-999,currentunitindex);
}

int32_t Game::NextUnitAtLocIndex(const int8_t civindex, const int32_t x, const int32_t y, const int32_t currentunitindex) const
{
	const int32_t startindex=(currentunitindex>=0 && currentunitindex<countof(m_gamedata.m_unit) ? currentunitindex : 0);
	int32_t idx;
	for(int32_t i=0,idx=startindex+1; i<countof(m_gamedata.m_unit); i++,idx++)
	{
		if(idx>=countof(m_gamedata.m_unit))
		{
			idx=0;
		}
		if(m_gamedata.m_unit[idx].owner==civindex && (m_gamedata.m_unit[idx].flags & UNIT_ALIVE)==UNIT_ALIVE && ((x==-999 && y==-999) || (m_gamedata.m_unit[idx].x==x && m_gamedata.m_unit[idx].y==y)))
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

void Game::EndPlayerTurn(const uint8_t civindex)
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

	// adjust movement points for all units
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
				DisbandUnit(-1,i,false);
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

		// if population is 0 - then make sure all units (except settlers) are dead
		if(m_gamedata.m_city[i].population==0)
		{
			for(size_t u=(i*UNITS_PER_CITY); u<(i*UNITS_PER_CITY)+UNITS_PER_CITY; u++)
			{
				if(m_gamedata.m_unit[u].type!=UNITTYPE_SETTLER)
				{
					m_gamedata.m_unit[u].flags=0;
				}
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
							// if barracks then set veteran
							if((m_gamedata.m_city[i].improvements & (0x01 << IMPROVEMENT_BARRACKS))==(0x01 << IMPROVEMENT_BARRACKS))
							{
								m_gamedata.m_unit[ui].flags|=UNIT_VETERAN;
							}
							m_gamedata.m_unit[ui].type=udi;
							m_gamedata.m_unit[ui].owner=m_gamedata.m_city[i].owner;
							m_gamedata.m_unit[ui].x=m_gamedata.m_city[i].x;
							m_gamedata.m_unit[ui].y=m_gamedata.m_city[i].y;
							m_gamedata.m_unit[ui].movesleft=unitdata[udi].moves;

							// TODO - clear bad move count for this unit

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

	// can't have more than 50000 gold per civ to prevent overflow
	for(size_t i=0; i<countof(m_gamedata.m_civ); i++)
	{
		if(m_gamedata.m_civ[i].gold>50000)
		{
			m_gamedata.m_civ[i].gold=50000;
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

void Game::CityProducingBuildCost(const uint8_t cityindex, uint32_t &resources, uint32_t &gold) const
{
	resources=0;
	gold=0;
	if(cityindex<countof(m_gamedata.m_city) && m_gamedata.m_city[cityindex].producing)
	{
		if(buildingxref[m_gamedata.m_city[cityindex].producing].buildingtype==BUILDINGTYPE_UNIT)
		{
			resources=unitdata[buildingxref[m_gamedata.m_city[cityindex].producing].building].buildresources;
			gold=unitdata[buildingxref[m_gamedata.m_city[cityindex].producing].building].buildgold;
		}
		else if(buildingxref[m_gamedata.m_city[cityindex].producing].buildingtype==BUILDINGTYPE_IMPROVEMENT)
		{
			resources=improvementdata[buildingxref[m_gamedata.m_city[cityindex].producing].building].buildresources;
			gold=improvementdata[buildingxref[m_gamedata.m_city[cityindex].producing].building].buildgold;
		}
		// TODO - other building type?
	}
}

bool Game::CityBuyProducing(const uint8_t cityindex)
{
	if(cityindex<countof(m_gamedata.m_city) && m_gamedata.m_city[cityindex].producing && m_gamedata.m_city[cityindex].owner==m_gamedata.m_currentcivturn)
	{
		uint32_t res;
		uint32_t gold;
		CityProducingBuildCost(cityindex,res,gold);
		if(gold<=m_gamedata.m_civ[m_gamedata.m_city[cityindex].owner].gold)
		{
			m_gamedata.m_city[cityindex].shields=res;
			m_gamedata.m_civ[m_gamedata.m_city[cityindex].owner].gold-=gold;
			return true;
		}
	}
	return false;
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

int32_t Game::CityUnitCount(const uint8_t cityindex) const
{
	int32_t count=0;
	for(size_t i=(cityindex*UNITS_PER_CITY); i<(cityindex*UNITS_PER_CITY)+UNITS_PER_CITY; i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE)
		{
			count++;
		}
	}
	return count;
}

bool Game::CanFoundCity(const uint8_t civindex, const int32_t settlerindex) const
{
	// unit type must be settler, we must be on land, no other city must be within 4 spaces radius, and we must have a free city slot for this civ
	if(civindex<countof(m_gamedata.m_civ) && settlerindex>=0 && settlerindex<countof(m_gamedata.m_unit))
	{
		const Unit *u=&(m_gamedata.m_unit[settlerindex]);
		if((u->flags & UNIT_ALIVE)==UNIT_ALIVE && u->type==UNITTYPE_SETTLER && u->owner==civindex && u->movesleft>0)
		{
			if(m_gamedata.m_map->GetBaseType(u->x,u->y)==BaseTerrain::BASETERRAIN_LAND && CityInRadius(-1,u->x,u->y,4)==-1)
			{
				if(FreeCityIndex(civindex)>-1)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool Game::FoundCity(const uint8_t civindex, const int32_t settlerindex)
{
	if(CanFoundCity(civindex,settlerindex)==true)
	{
		m_gamedata.m_unit[settlerindex].flags=0;

		int8_t ci=FreeCityIndex(civindex);
		memset(&(m_gamedata.m_city[ci]),0,sizeof(City));
		m_gamedata.m_city[ci].x=m_gamedata.m_unit[settlerindex].x;
		m_gamedata.m_city[ci].y=m_gamedata.m_unit[settlerindex].y;
		m_gamedata.m_city[ci].owner=civindex;
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
			// TODO - reset bad move count for this unit

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

CityProduction Game::GetTerrainProduction(const uint8_t civindex, const int32_t x, const int32_t y, const bool ignoreenemy) const
{
	CityProduction prod;
	memset(&prod,0,sizeof(CityProduction));
	MapCoord mc(m_gamedata.m_map->Width(),m_gamedata.m_map->Height(),0,0);
	// 1st get possible resource production of every tile in city
	int32_t idx=0;
	for(int32_t dy=-2; dy<3; dy++)
	{
		for(int32_t dx=-2; dx<3; dx++,idx++)
		{
			mc.Set(dx+((int32_t)x),dy+((int32_t)y));
			TerrainTile tt=m_gamedata.m_map->GetTile(mc.X(),mc.Y());
			prod.tile[idx].food+=TerrainTile::terrainproduction[tt.Type()].food+TerrainTile::resourceproduction[tt.Resource()].food;
			prod.tile[idx].resources+=TerrainTile::terrainproduction[tt.Type()].resources+TerrainTile::resourceproduction[tt.Resource()].resources;
		
			// if there's an enemy on this tile - then we don't get any production from it
			if(ignoreenemy==false && UnitIndexAtLocation(-1,mc.X(),mc.Y())>=0)
			{
				if(m_gamedata.m_unit[UnitIndexAtLocation(-1,mc.X(),mc.Y())].owner!=civindex)
				{
					prod.tile[idx].food=0;
					prod.tile[idx].resources=0;
				}
			}
		
		}
	}
	return prod;
}

CityProduction Game::GetCityProduction(const int32_t cityidx) const
{
	CityProduction prod;

	if(cityidx>=0 && cityidx<countof(m_gamedata.m_city) && m_gamedata.m_city[cityidx].population>0)
	{
		const City *c=&(m_gamedata.m_city[cityidx]);

		prod=GetTerrainProduction(c->owner,c->x,c->y,false);

		// city center always starts with +1 food and +1 resource no matter underlying terrain
		prod.tile[12].food+=1;
		prod.tile[12].resources+=1;

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

		// if city has factory then 50% more resources;
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

bool Game::EmbarkableShipAtLocation(const uint8_t civindex, const int32_t x, const int32_t y) const
{
	const int32_t si=ShipAtLocation(x,y);
	if(si>=0)
	{
		const Unit *u=&(m_gamedata.m_unit[si]);
		if(u->owner==civindex)
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

int32_t Game::EnemyShipAtLocation(const uint8_t civindex, const int32_t x, const int32_t y) const
{
	const int32_t si=ShipAtLocation(x,y);
	return (si>=0 && m_gamedata.m_unit[si].owner!=civindex ? si : -1);
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
bool Game::MoveUnit(const uint8_t civindex, const int32_t unitindex, const int32_t dx, const int32_t dy)
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





	if((dx!=0 || dy!=0) && m_gamedata.m_currentcivturn==civindex && unitindex>=0 && unitindex<countof(m_gamedata.m_unit) && m_gamedata.m_unit[unitindex].owner==civindex && (m_gamedata.m_unit[unitindex].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[unitindex].movesleft>0)
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
		if(desttile.BaseType()==BaseTerrain::BASETERRAIN_WATER && EnemyShipAtLocation(civindex,mc.X(),mc.Y())>=0)
		{
			eu=&(m_gamedata.m_unit[EnemyShipAtLocation(civindex,mc.X(),mc.Y())]);
			eucount=1;	// only ship counts because when it's destoryed all the embarked units are destroyed
		}
		// check for enemy city at destination
		City *ec=nullptr;
		int32_t eci=CityIndexAtLocation(mc.X(),mc.Y());
		if(eci>=0 && m_gamedata.m_city[eci].owner!=civindex)
		{
			ec=&(m_gamedata.m_city[eci]);
		}

		bool trymove=true;
		bool attackok=false;

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

		if(attackok)
		{
			float defmult=1.0;
			
			// if enemy city, but no units, then we take the city
			if(ec && !eu)
			{
				int32_t mc=FreeCityIndex(ec->owner);		// if we need to move settler units to a new city
				ec->owner=civindex;
				ec->producing=0;
				ec->shields=0;
				// clear out enemy units with this home city
				for(size_t i=(eci*UNITS_PER_CITY); i<(eci*UNITS_PER_CITY)+UNITS_PER_CITY; i++)
				{
					// try to move settler units to an unused civ city so they won't be killed
					if(mc>=0 && m_gamedata.m_unit[i].flags!=0 && m_gamedata.m_unit[i].type==UNITTYPE_SETTLER)
					{
						int32_t ni=FreeUnitIndex(mc);
						if(ni>=0)
						{
							m_gamedata.m_unit[ni]=m_gamedata.m_unit[i];
						}
					}
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
				}
				// defender died (there may be other defender units still there)
				else
				{
					trymove=true;
					u->flags|=UNIT_VETERAN;
					AddSpriteOverlay(eu->x,eu->y,SpriteSheetPos(2,0),60);
					DisbandUnit(-1,eu-m_gamedata.m_unit,true);

					// attacking unit was in a city - don't move them out of city and subtract movement point
					if(CityIndexAtLocation(u->x,u->y)>=0)
					{
						trymove=false;
						if(u->movesleft>0)
						{
							u->movesleft--;
						}
					}

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

				}
			}
		}

		if(trymove==true && u->movesleft>0 && 
			(
			// land unit on water can move to land (without any enemy unit or city)
			(unitterrain==BaseTerrain::BASETERRAIN_LAND && sourcetile.BaseType()==BaseTerrain::BASETERRAIN_WATER && desttile.BaseType()==BaseTerrain::BASETERRAIN_LAND && !eu && !ec)
			||
			// land unit on land can move (as long as no enemy units in dest - attack above would have skipped moving if so)
			(unitterrain==BaseTerrain::BASETERRAIN_LAND && sourcetile.BaseType()==BaseTerrain::BASETERRAIN_LAND && desttile.BaseType()==BaseTerrain::BASETERRAIN_LAND)
			|| 
			// land unit can move on embarkable ship
			(unitterrain==BaseTerrain::BASETERRAIN_LAND && desttile.BaseType()==BaseTerrain::BASETERRAIN_WATER && EmbarkableShipAtLocation(civindex,mc.X(),mc.Y())==true)
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
			if(unitterrain==BaseTerrain::BASETERRAIN_LAND && sourcetile.BaseType()!=desttile.BaseType())
			{
				u->movesleft=0;
			}

			// TODO - reset bad move count for this unit

			return true;
		}
		// TODO - unit couldn't move - keep track of each unit bad movement count - if AI has too many bad moves in a row - disband it
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

void Game::HandleAI(const uint8_t civindex)
{
	//trace("AI");

	// TODO - unit logic
	int32_t settlercount=0;
	int32_t watercount=0;
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[i].owner==civindex)
		{
			if(m_gamedata.m_unit[i].type==UNITTYPE_SETTLER)
			{
				AISettlerUnit(i);
				settlercount++;
			}
			else if((unitdata[m_gamedata.m_unit[i].type].flags & UNITDATA_MOVE_LAND) == UNITDATA_MOVE_LAND)
			{
				AIMilitaryLandUnit(i);
			}
			else if((unitdata[m_gamedata.m_unit[i].type].flags & UNITDATA_MOVE_WATER) == UNITDATA_MOVE_WATER)
			{
				AIMilitaryWaterUnit(i);
				watercount++;
				if(watercount>5)
				{
					DisbandUnit(-1,i,false);
				}
			}
		}
	}

	// TODO - city logic
	bool buildingsettler=false;
	bool buildingwaterunit=false;

	//first see if any cities are already building settler
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0 && m_gamedata.m_city[i].owner==civindex)
		{
			if(m_gamedata.m_city[i].producing==BUILDING_SETTLER)
			{
				buildingsettler=true;
			}
		}
	}

	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0 && m_gamedata.m_city[i].owner==civindex)
		{
			City *c=&(m_gamedata.m_city[i]);
			int32_t cei=ClosestEnemyUnit(c->owner,c->x,c->y,false);
			// if we're building something and it's <50% resouces completed, and the civ has 4x gold needed to buy it, then buy it
			uint32_t res=0;
			uint32_t gold=0;
			CityProducingBuildCost(i,res,gold);
			if(c->producing!=0 && c->shields*2<=res && gold*4<=m_gamedata.m_civ[c->owner].gold)
			{
				CityBuyProducing(i);
			}
			// if we don't have any units for this city - build a land military unit depending on production
			// or if there's an enemy closer than 10 spots and we have a spare unit spot and pop at least 2x more than unit count, create a military unit
			else if(CityUnitCount(i)==0 || (CityUnitCount(i)<UNITS_PER_CITY && c->population>((UNITS_PER_CITY-CityUnitCount(i))*2) && cei>=0 && Distance2(c->x,c->y,m_gamedata.m_unit[cei].x,m_gamedata.m_unit[cei].y)<=10))
			{
				// set to militia to get resource production
				c->producing=BUILDING_MILITIA;
				CityProduction cprod=GetCityProduction(i);
				if(cprod.totalresources>20)
				{
					c->producing=BUILDING_KNIGHT;
				}
				else if(cprod.totalresources>12)
				{
					c->producing=BUILDING_HORSEMAN;
				}
				else if(cprod.totalresources>6)
				{
					c->producing=BUILDING_PHALANX;
				}
			}
			// if we have a spare city slot and city pop is >2 and we don't have more than 2 settlers currnetly - build a settler if open unit slot
			else if(c->population>2 && buildingsettler==false && settlercount<2 && FreeCityIndex(c->owner)>=0 && CityUnitCount(i)<UNITS_PER_CITY)
			{
				c->producing=BUILDING_SETTLER;
				buildingsettler=true;
			}
			// if our population >3, build a granary
			else if(c->population>3 && (c->improvements & (0x01 << IMPROVEMENT_GRANARY)) != (0x01 << IMPROVEMENT_GRANARY))
			{
				c->producing=BUILDING_GRANARY;
			}
			else if(c->population>4 && (c->improvements & (0x01 << IMPROVEMENT_BARRACKS)) != (0x01 << IMPROVEMENT_BARRACKS))
			{
				c->producing=BUILDING_BARRACKS;
			}
			// if our population >5, build a market
			else if(c->population>5 && (c->improvements & (0x01 << IMPROVEMENT_MARKET)) != (0x01 << IMPROVEMENT_MARKET))
			{
				c->producing=BUILDING_MARKET;
			}
			// if our population >8, build a bank
			else if(c->population>8 && (c->improvements & (0x01 << IMPROVEMENT_BANK)) != (0x01 << IMPROVEMENT_BANK))
			{
				c->producing=BUILDING_BANK;
			}
			else if(c->population>9 && (c->improvements & (0x01 << IMPROVEMENT_FACTORY)) != (0x01 << IMPROVEMENT_FACTORY))
			{
				c->producing=BUILDING_FACTORY;
			}
			else if(c->population>11 && (c->improvements & (0x01 << IMPROVEMENT_CITYWALLS)) != (0x01 << IMPROVEMENT_CITYWALLS))
			{
				c->producing=BUILDING_CITYWALLS;
			}
			else if(c->population>13 && (c->improvements & (0x01 << IMPROVEMENT_AQUEDUCT)) != (0x01 << IMPROVEMENT_AQUEDUCT))
			{
				c->producing=BUILDING_AQUEDUCT;
			}
			// random military unit (population at least 2x number of existing units)
			else if(FreeUnitIndex(i)>=0 && c->producing==0 && c->population>((UNITS_PER_CITY-CityUnitCount(i))*2))
			{
				RandomMT rand;
				rand.Seed(m_gamedata.m_ticks + (i << 16));
				do
				{
					c->producing=(rand.NextDouble()*BUILDING_UNIT_MAX)+1;
				}while(c->producing==BUILDING_SETTLER);		// don't build a random settler

				// check if it's a water unit, but we don't have water nearby
				if((unitdata[buildingxref[c->producing].building].flags & UNITDATA_MOVE_WATER) == UNITDATA_MOVE_WATER)
				{
					bool haswater=false;
					for(int32_t dy=-1; dy<2; dy++)
					{
						for(int32_t dx=-1; dx<2; dx++)
						{
							if(m_gamedata.m_map->GetBaseType(((int32_t)c->x)+dx,((int32_t)c->y)+dy)==BaseTerrain::BASETERRAIN_WATER)
							{
								haswater=true;
							}
						}
					}
					// reset the unit - it will select a new unit next turn
					if(haswater==false || watercount>5)
					{
						c->producing=0;
					}
				}
			}
			// no free unit slots - no improvements left
			else
			{
				c->producing=0;
			}
			
		}

	}

}

/*
void Game::AIRandomMove(const uint32_t unitindex, const int generaldirection, const bool forcemove, const uint64_t extrarandom)
{
	bool retry=false;
	int32_t cnt=0;
	do
	{
	
		RandomMT rand(m_gamedata.m_ticks + extrarandom + ((unitindex << 24) | (m_gamedata.m_unit[unitindex].x << 16) | m_gamedata.m_unit[unitindex].y) + cnt);

		float ns=0;		// north/south bias
		float ew=0;		// east/west bias

		if(generaldirection==DIR_NORTHWEST || generaldirection==DIR_NORTH || generaldirection==DIR_NORTHEAST)
		{
			ns-=0.85;
		}
		if(generaldirection==DIR_NORTHEAST || generaldirection==DIR_EAST || generaldirection==DIR_SOUTHEAST)
		{
			ew+=0.85;
		}
		if(generaldirection==DIR_SOUTHEAST || generaldirection==DIR_SOUTH || generaldirection==DIR_SOUTHWEST)
		{
			ns+=0.85;
		}
		if(generaldirection==DIR_SOUTHWEST || generaldirection==DIR_WEST || generaldirection==DIR_NORTHWEST)
		{
			ew-=0.85;
		}

		int32_t dx=((rand.NextDouble()*4.0)-2.0)+ew;
		int32_t dy=((rand.NextDouble()*4.0)-2.0)+ns;

		dx=dx<-1 ? -1 : (dx>1 ? 1 : dx);
		dy=dy<-1 ? -1 : (dy>1 ? 1 : dy);

		if(dx!=0 || dy!=0)
		{
			MoveUnit(m_gamedata.m_unit[unitindex].owner,unitindex,dx,dy);
			retry=false;
		}
		else
		{
			retry=true;
			cnt++;
		}

	}while(retry==true && forcemove==true && cnt<100);

}
*/

void Game::AIRandomMove(const uint32_t unitindex, const int generaldirection, const bool forcemove, const uint64_t extrarandom)
{
    const uint8_t dirs[8]={DIR_NORTHWEST,DIR_NORTH,DIR_NORTHEAST,DIR_EAST,DIR_SOUTHEAST,DIR_SOUTH,DIR_SOUTHWEST,DIR_WEST};
    const int8_t dx[8]={-1,0,1,1,1,0,-1,-1};
    const int8_t dy[8]={-1,-1,-1,0,1,1,1,0};
    const float chance[8]={0.6,0.1,0.05,0.025,0.015,0.025,0.05,0.1};
	bool retry=false;
	int32_t cnt=0;
	uint8_t offset=0;

    if(generaldirection!=DIR_NONE)
    {
        for(size_t i=0; i<8; i++)
        {
            if(dirs[i]==generaldirection)
            {
                offset=i;
            }
        }
    }

	do
	{
		RandomMT rand(m_gamedata.m_ticks + extrarandom + ((unitindex << 24) | (m_gamedata.m_unit[unitindex].x << 16) | m_gamedata.m_unit[unitindex].y) + cnt);
		const float r=rand.NextDouble();

		float c=0;
    	uint8_t i=0;

		do
		{
			if(generaldirection==DIR_NONE)
			{
				c+=0.12125;
			}
			else
			{
				c+=chance[i];
			}
		}while(c<=r && ++i<8);

		if(i<8)
		{
			MoveUnit(m_gamedata.m_unit[unitindex].owner,unitindex,dx[(i+offset)%8],dy[(i+offset)%8]);
			retry=false;
		}
		else
		{
			retry=true;
			cnt++;
		}

	}while(retry==true && forcemove==true && cnt<100);
}

void Game::AIMoveDirection(const uint32_t unitindex, const int direction)
{
	int32_t dx=0;
	int32_t dy=0;

	switch(direction)
	{
	case DIR_NORTHWEST:
		dx=-1;
		dy=-1;
		break;
	case DIR_NORTH:
		dy=-1;
		break;
	case DIR_NORTHEAST:
		dx=1;
		dy=-1;
		break;
	case DIR_WEST:
		dx=-1;
		break;
	case DIR_EAST:
		dx=1;
		break;
	case DIR_SOUTHWEST:
		dx=-1;
		dy=1;
		break;
	case DIR_SOUTH:
		dy=1;
		break;
	case DIR_SOUTHEAST:
		dx=1;
		dy=1;
		break;
	}

	MoveUnit(m_gamedata.m_unit[unitindex].owner,unitindex,dx,dy);
}

void Game::AISettlerUnit(const uint32_t unitindex)
{
	Unit *u=&(m_gamedata.m_unit[unitindex]);
	const TerrainTile sourcetile=m_gamedata.m_map->GetTile(u->x,u->y);

	// if there's no spare city slot - disband unit
	// TODO - we could move to nearest city and expand population
	if(FreeCityIndex(u->owner)<0)
	{
		DisbandUnit(-1,unitindex,false);
		return;
	}
	// we've moved too far north or south - disband unit
	if(u->x<6 || u->y>127-6)
	{
		DisbandUnit(-1,unitindex,false);
		return;
	}

	// if there's an enemy closer than 5 units - try to move away
	int32_t ce=ClosestEnemyUnit(u->owner,u->x,u->y,false);
	if(ce>=0 && Distance2(u->x,u->y,m_gamedata.m_unit[ce].x,m_gamedata.m_unit[ce].y)<=5)
	{
		AIRandomMove(unitindex,Direction(m_gamedata.m_unit[ce].x,m_gamedata.m_unit[ce].y,u->x,u->y),true,0);
		return;
	}
	/* our code below searching for best spot should take care of moving away from city now - otherwise we'll randomly move and eventually get to a spot we can build a city
	// if there's a city closer than 5 units - try to move away
	int32_t cc=CityInRadius(-1,u->x,u->y,5);
	if(cc>=0)
	{
		// default dir away from city
		uint8_t dir=Direction(m_gamedata.m_city[cc].x,m_gamedata.m_city[cc].y,u->x,u->y);
		// if we're already too far north or south, move towards equator
		if(u->y<22)
		{
			dir=DIR_SOUTH;
		}
		else if(u->y>(127-22))
		{
			dir=DIR_NORTH;
		}
		// TODO - make sure direction has land to move onto

		AIRandomMove(unitindex,dir,true,0);
		return;
	}
	*/

	// find best neighboring tile
	CityProduction bestprod;
	int32_t bestdx=-99999;
	int32_t bestdy=-99999;
	bestprod.totalfood=0;
	bestprod.totalresources=0;
	bestprod.totalgold=0;
	for(int32_t dy=-6; dy<7; dy++)
	{
		for(int32_t dx=-6; dx<7; dx++)
		{
			if(CityInRadius(-1,((int32_t)u->x)+dx,((int32_t)u->y)+dy,5)<0 && m_gamedata.m_map->GetBaseType(((int32_t)u->x)+dx,((int32_t)u->y)+dy)==BaseTerrain::BASETERRAIN_LAND && m_gamedata.m_pathfinder->DirectConnection(u->x,u->y,((int32_t)u->x)+dx,((int32_t)u->y)+dy)==true)
			{
				CityProduction tprod=GetTerrainProduction(u->owner,((int32_t)u->x)+dx,((int32_t)u->y)+dy,true);
				for(size_t i=0; i<countof(tprod.tile); i++)
				{
					tprod.totalfood+=tprod.tile[i].food;
					tprod.totalresources+=tprod.tile[i].resources;
				}
				// if water in radius - adjust gold so we prefer tile next to water
				if(BaseTerrainInRadius(((int32_t)u->x)+dx,((int32_t)u->y)+dy,1,BaseTerrain::BASETERRAIN_WATER))
				{
					tprod.totalgold+=20;
				}
				// gold may be 0, so we skip that >0
				if(tprod.totalfood>10 && tprod.totalresources>10 && (tprod.totalfood+tprod.totalresources+tprod.totalgold)>(bestprod.totalfood+bestprod.totalresources+bestprod.totalgold))
				{
					bestprod=tprod;
					bestdx=dx;
					bestdy=dy;
				}
			}
		}
	}
	if(bestdx>-99999 && bestdy>-99999)
	{
		// we're on the spot with the best resource production - build the city
		if(bestdx==0 && bestdy==0)
		{
			FoundCity(u->owner,unitindex);
			return;
		}
		// move to the spot with the better resources
		else
		{
			// single space move
			if(bestdx>-2 && bestdx<2 && bestdy>-2 && bestdy<2)
			{
				MoveUnit(u->owner,unitindex,bestdx,bestdy);
			}
			// multiple space move
			else
			{
				AIMoveDirection(unitindex,Direction(u->x,u->y,((int32_t)u->x)+bestdx,((int32_t)u->y)+bestdy));
			}
			return;
		}
	}
	// did not find any good spots for city - move randomly
	else
	{
		AIRandomMove(unitindex,DIR_NONE,true,0);
		return;
	}

}

void Game::AIMilitaryLandUnit(const uint32_t unitindex)
{
	// TODO - finish

	Unit *u=&(m_gamedata.m_unit[unitindex]);
	RandomMT rand;
	rand.Seed(m_gamedata.m_ticks + ((unitindex << 24) | (u->x << 16) | u->y));
	bool wait=false;
	int32_t cnt=0;		// use for extra seed for random so we don't get stuck in a loop trying the same failed movement

	while((u->flags & UNIT_ALIVE) == UNIT_ALIVE && u->movesleft>0 && wait==false && cnt<20)
	{
		const int32_t cei=ClosestEnemyUnit(u->owner,u->x,u->y,true);
		const int32_t cfi=ClosestFriendlyUnit(u->owner,u->x,u->y);
		const int32_t cec=ClosestEnemyCity(u->owner,u->x,u->y,true);
		const int32_t cfc=ClosestFriendlyCity(u->owner,u->x,u->y);

		const int32_t ceidist=cei>=0 ? Distance2(u->x,u->y,m_gamedata.m_unit[cei].x,m_gamedata.m_unit[cei].y) : -1;
		const int32_t cfidist=cfi>=0 ? Distance2(u->x,u->y,m_gamedata.m_unit[cfi].x,m_gamedata.m_unit[cfi].y) : -1;
		const int32_t cecdist=cec>=0 ? Distance2(u->x,u->y,m_gamedata.m_city[cec].x,m_gamedata.m_city[cec].y) : -1;
		const int32_t cfcdist=cfc>=0 ? Distance2(u->x,u->y,m_gamedata.m_city[cfc].x,m_gamedata.m_city[cfc].y) : -1;

		const bool incity=CityIndexAtLocation(u->x,u->y)>=0 ? true : false;

		// next to enemy unit - chance to attack
		if(cei>=0 && ceidist==1)
		{
			if(rand.NextDouble()<0.5)
			{
				AIMoveDirection(unitindex,Direction(u->x,u->y,m_gamedata.m_unit[cei].x,m_gamedata.m_unit[cei].y));
			}
			return;
		}
		// enemy unit and friendly city in radius - fall back to friendly city
		else if(cei>=0 && cfc>=0 && ceidist<10 && cfcdist<10)
		{
			// move towards friendly city if we're not already in a city
			if(incity==false)
			{
				AIMoveDirection(unitindex,Direction(u->x,u->y,m_gamedata.m_city[cfc].x,m_gamedata.m_city[cfc].y));
			}
			else
			{
				wait=true;
			}
		}
		// enemy unit in radius but no friendly unit, stay put
		else if(cei>=0 && ceidist<10 && (cfi==-1 || cfidist>10))
		{
			wait=true;
		}
		// enemy unit in radius and at least 1 friendly unit in radius, move towards enemy
		else if(cei>=0 && ceidist<10 && cfi>=0 && cfidist<10)
		{
			if(ceidist==1)
			{
				AIMoveDirection(unitindex,Direction(u->x,u->y,m_gamedata.m_unit[cei].x,m_gamedata.m_unit[cei].y));
			}
			else
			{
				AIRandomMove(unitindex,Direction(u->x,u->y,m_gamedata.m_unit[cei].x,m_gamedata.m_unit[cei].y),true,cnt);
			}
		}
		// enemy city in radius and no friendly unit, stay put
		else if(cec>=0 && cecdist<10 && (cfi==-1 || cfidist>10))
		{
			wait=true;
		}
		// enemy city in radius and at least 1 friendly unit in radius, move towards city
		else if(cec>=0 && cecdist<10 && cfi>=0 && cfidist<10)
		{
			if(cecdist==1)
			{
				AIMoveDirection(unitindex,Direction(u->x,u->y,m_gamedata.m_city[cec].x,m_gamedata.m_city[cec].y));
			}
			else
			{
				AIRandomMove(unitindex,Direction(u->x,u->y,m_gamedata.m_city[cec].x,m_gamedata.m_city[cec].y),true,cnt);
			}
		}
		// if nothing else going on, every other unit will try to move towards enemy city
		else if(cec>=0 && (unitindex%2)==0)
		{
			// default dir to general direction - use pathfinding to get real direction needed (if for some reason pathfind fails, the general direction was set)
			uint8_t dir=Direction(u->x,u->y,m_gamedata.m_city[cec].x,m_gamedata.m_city[cec].y);
			m_gamedata.m_pathfinder->Pathfind(u->x,u->y,m_gamedata.m_city[cec].x,m_gamedata.m_city[cec].y,dir);
			AIRandomMove(unitindex,dir,false,cnt);
		}
		else
		{
			if(rand.NextDouble()<0.5)
			{
				AIRandomMove(unitindex,DIR_NONE,false,cnt);
			}
			else
			{
				wait=true;
			}
		}

		// militia unit with no more free city unit slots - disband militia to make room for a different unit
		if(u->type==UNITTYPE_MILITIA && FreeUnitIndex(unitindex/UNITS_PER_CITY)<0)
		{
			DisbandUnit(-1,unitindex,false);
		}

		cnt++;

	}

}

void Game::AIMilitaryWaterUnit(const uint32_t unitindex)
{
	// TODO - finish

	Unit *u=&(m_gamedata.m_unit[unitindex]);

	//if we're a water unit - but no water nearby, disband
	if(BaseTerrainInRadius(u->x,u->y,1,BaseTerrain::BASETERRAIN_WATER)==false)
	{
		DisbandUnit(-1,unitindex,false);
		return;
	}

	const int32_t cei=ClosestEnemyUnit(u->owner,u->x,u->y,true);

	// if we're in a city - move out to water
	if(m_gamedata.m_map->GetBaseType(u->x,u->y)==BaseTerrain::BASETERRAIN_LAND)
	{
		// TODO - find direction of water
		AIRandomMove(unitindex,DIR_NONE,true,0);
	}
	// if there's an enemy we can reach, move towards them
	else if(cei>=0)
	{
		if(Distance2(u->x,u->y,m_gamedata.m_unit[cei].x,m_gamedata.m_unit[cei].y)>1)
		{
			uint8_t dir=DIR_NONE;
			m_gamedata.m_pathfinder->Pathfind(u->x,u->y,m_gamedata.m_unit[cei].x,m_gamedata.m_unit[cei].y,dir);
			AIRandomMove(unitindex,dir,true,0);
		}
		else
		{
			AIMoveDirection(unitindex,Direction(u->x,u->y,m_gamedata.m_unit[cei].x,m_gamedata.m_unit[cei].y));
		}
	}
	else
	{
		AIRandomMove(unitindex,DIR_NONE,true,0);
	}

	// find location where our land units are congregated and go to rally point on land/water border

	// if we have full units on board, move towards enemy congregated units

}

int32_t Game::ClosestEnemyUnit(const uint8_t civindex, const int32_t x, const int32_t y, const bool musthavepath) const
{
	int32_t closest=-1;
	int32_t closestdist=128*128;
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[i].owner!=civindex && (closest==-1 || Distance2(x,y,m_gamedata.m_unit[i].x,m_gamedata.m_unit[i].y)<closestdist))
		{
			uint8_t dir;
			if(musthavepath==false || (m_gamedata.m_pathfinder->Pathfind(x,y,m_gamedata.m_unit[i].x,m_gamedata.m_unit[i].y,dir)==true))
			{
				closest=i;
				closestdist=Distance2(x,y,m_gamedata.m_unit[i].x,m_gamedata.m_unit[i].y);
			}
		}
	}
	return closest;
}

int32_t Game::ClosestFriendlyUnit(const uint8_t civindex, const int32_t x, const int32_t y) const
{
	int32_t closest=-1;
	int32_t closestdist=128*128;
	for(size_t i=0; i<countof(m_gamedata.m_unit); i++)
	{
		if((m_gamedata.m_unit[i].flags & UNIT_ALIVE) == UNIT_ALIVE && m_gamedata.m_unit[i].owner==civindex && (closest==-1 || Distance2(x,y,m_gamedata.m_unit[i].x,m_gamedata.m_unit[i].y)<closestdist))
		{
			closest=i;
			closestdist=Distance2(x,y,m_gamedata.m_unit[i].x,m_gamedata.m_unit[i].y);
		}
	}
	return closest;
}

int32_t Game::ClosestEnemyCity(const uint8_t civindex, const int32_t x, const int32_t y, const bool musthavepath) const
{
	int32_t closest=-1;
	int32_t closestdist=128*128;
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0 && m_gamedata.m_city[i].owner!=civindex && (closest==-1 || Distance2(x,y,m_gamedata.m_city[i].x,m_gamedata.m_city[i].y)<closestdist))
		{
			uint8_t dir;
			if(musthavepath==false || (m_gamedata.m_pathfinder->Pathfind(x,y,m_gamedata.m_city[i].x,m_gamedata.m_city[i].y,dir)==true))
			{
				closest=i;
				closestdist=Distance2(x,y,m_gamedata.m_city[i].x,m_gamedata.m_city[i].y);
			}
		}
	}
	return closest;
}

int32_t Game::ClosestFriendlyCity(const uint8_t civindex, const int32_t x, const int32_t y) const
{
	int32_t closest=-1;
	int32_t closestdist=128*128;
	for(size_t i=0; i<countof(m_gamedata.m_city); i++)
	{
		if(m_gamedata.m_city[i].population>0 && m_gamedata.m_city[i].owner==civindex && (closest==-1 || Distance2(x,y,m_gamedata.m_city[i].x,m_gamedata.m_city[i].y)<closestdist))
		{
			closest=i;
			closestdist=Distance2(x,y,m_gamedata.m_city[i].x,m_gamedata.m_city[i].y);
		}
	}
	return closest;
}

int32_t Game::Distance2(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2) const
{
	// distance will be greater of absolute distance between x coords or y coords

	// wrap x coord (distance can't be more than 1/2 map width)
	int32_t dx=(x2-x1);
	dx=(dx<0 ? -dx : dx);
	dx=(dx>m_gamedata.m_map->Width()/2 ? m_gamedata.m_map->Width()-dx : dx);
	dx=(dx<0 ? -dx : dx);

	int32_t dy=(y2-y1);
	dy=(dy<0 ? -dy : dy);

	return dx>dy ? dx : dy;
}

int Game::Direction(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2) const
{
	MapCoord mc(m_gamedata.m_map->Width(),m_gamedata.m_map->Height(),0,0);

	int32_t dy=y2-y1;
	int32_t dx=(x2-x1);
	if(dx<0)
	{
		dx=dx<-(m_gamedata.m_map->Width()/2) ? -(m_gamedata.m_map->Width()+dx) : dx;
	}
	else
	{
		dx=dx>(m_gamedata.m_map->Width()/2) ? m_gamedata.m_map->Width()-dx : dx;
	}

	if(dy<0)
	{
		if(dx<0)
		{
			return DIR_NORTHWEST;
		}
		else if(dx>0)
		{
			return DIR_NORTHEAST;
		}
		else
		{
			return DIR_NORTH;
		}
	}
	else if(dy>0)
	{
		if(dx<0)
		{
			return DIR_SOUTHWEST;
		}
		else if(dx>0)
		{
			return DIR_SOUTHEAST;
		}
		else
		{
			return DIR_SOUTH;
		}
	}
	else if(dx<0)
	{
		return DIR_WEST;
	}
	else if(dx>0)
	{
		return DIR_EAST;
	}

	return DIR_NONE;

}

bool Game::BaseTerrainInRadius(const int32_t x, const int32_t y, const int32_t r, const BaseTerrain::TerrainType terrain) const
{
	for(int32_t dy=-r; dy<r+1; dy++)
	{
		for(int32_t dx=-r; dx<r+1; dx++)
		{
			if(m_gamedata.m_map->GetBaseType(x+dx,y+dy)==terrain)
			{
				return true;
			}
		}
	}
	return false;
}
