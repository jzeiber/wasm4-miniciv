#pragma once

#include <stdint.h>

#include "istate.h"
#include "iupdatable.h"
#include "idrawable.h"
#include "iinputhandler.h"
#include "gamedata.h"
#include "iplayerstate.h"

class Game:public IUpdatable,public IDrawable,public IInputHandler
{
public:
	Game();
	~Game();
	
	enum GameState
	{
		STATE_STARTUP=0,
		STATE_MAINMENU,
		STATE_PREGAME,
		STATE_GAME,
		STATE_MAX
	};

	struct SpriteOverlay
	{
		int32_t mapx;
		int32_t mapy;
		SpriteSheetPos sprite;
		int64_t ticks;
	};
	
	bool HandleInput(const Input *input, const uint8_t nothing);
	void Update(const int ticks, const uint8_t nothing, Game *game=nullptr);
	void Draw(const uint8_t nothing);

	void ChangeState(const uint8_t playerindex, const uint8_t newstate, const IStateChangeParams *params);

	uint64_t GetTicks() const;

	int8_t PlayerIndex() const;

	IState *GetPlayerState(const uint8_t playerindex);

	uint8_t PlayerCount() const;

	GameData &GetGameData();

	bool IsPlayerTurn(const uint8_t playerindex) const;
	void EndPlayerTurn(const uint8_t playerindex);
	int64_t TurnTicksLeft() const;

	int8_t PlayerCivIndex(const int8_t playerindex) const;															// return civ index for player index

	int32_t NextUnitIndex(const int8_t civindex, const int32_t currentunitindex, bool skipsentry) const;			// -1 for not found
	int32_t NextUnitAtLocIndex(const int8_t civindex, const int32_t x, const int32_t y, const int32_t currentunitindex, const bool skipsentry) const;	// -1 for not found
	int32_t NextCityIndex(const int8_t civindex, const int32_t currentcityindex) const;								// -1 for not found
	int32_t UnitIndexAtLocation(const int8_t owneridx, const int32_t x, const int32_t y) const;						// pass -1 for owner for any civ.  returns first unit idx at location, -1 if not found
	int32_t CityIndexAtLocation(const int32_t x, const int32_t y) const;											// returns city idx at location, -1 if not found
	int32_t CityInRadius(const int8_t owneridx, const int32_t x, const int32_t y, const int32_t radius) const;		// pass -1 for owner for any civ.  returns city idx within radius, -1 if not found
	int32_t CityFoodStorage(const int32_t cityindex) const;
	int32_t CityGrowthFoodRequired(const int32_t cityindex) const;													// returns amount of food needed to increase population
	int32_t FreeCityIndex(const int8_t owneridx) const;																// returns next free city index for owner, -1 if no slot available
	void CityProducingBuildCost(const uint8_t cityindex, uint32_t &resources, uint32_t &gold) const;
	bool CityBuyProducing(const uint8_t cityindex);
	int32_t CityUnitCount(const uint8_t cityindex) const;
	int32_t FreeUnitIndex(const int32_t cityindex) const;															// returns free unit index for city, -1 if not free slots available

	bool CanFoundCity(const uint8_t civindex, const int32_t settlerindex) const;
	bool FoundCity(const uint8_t civindex, const int32_t settlerindex);
	bool ExpandCity(const uint8_t playerindex, const int32_t settlerindex);
	bool CityCanExpand(const int32_t cityidx) const;

	int32_t ShipAtLocation(const int32_t x, const int32_t y) const;											// returns index of water movement unit at coord, otherwise -1
	bool EmbarkableShipAtLocation(const uint8_t civindex, const int32_t x, const int32_t y) const;
	int32_t EnemyShipAtLocation(const uint8_t civindex, const int32_t x, const int32_t y) const;			// returns index of enemy ship, otherwise -1 if no enemy ship
	int32_t UnitEmbarkedShipIndex(const int32_t unitindex) const;											// returns unit index of ship the unit is embarked on, or -1 if not embarked on a ship
	bool MoveUnit(const uint8_t civindex, const int32_t unitindex, const int32_t dx, const int32_t dy);
	bool DisbandUnit(const int8_t playerindex, const int32_t unitindex, const bool killed);
	int32_t UnitCountAtLocation(const uint32_t x, const int32_t y) const;

	SpriteSheetPos GetCitySpriteSheetPos(const int32_t cityidx) const;

	CityProduction GetTerrainProduction(const uint8_t civindex, const int32_t x, const int32_t y, const bool ignoreenemy) const;
	CityProduction GetCityProduction(const int32_t cityidx) const;

	void AddSpriteOverlay(const int32_t mapx, const int32_t mapy, const SpriteSheetPos spos, const int64_t ticks);
	void DrawSpriteOverlay(const int32_t mapx, const int32_t mapy, const int32_t screenx, const int32_t screeny);

	bool CivilizationAlive(const uint8_t civindex) const;		// checks for at least 1 city or 1 alive unit beloning to civilization

private:

	struct changestate
	{
		int8_t m_newstate;
		IStateChangeParams *m_params;
	};

	IPlayerState *m_playerstate[MAX_CIVILIZATIONS];
	changestate m_changestate[MAX_CIVILIZATIONS];
	GameData m_gamedata;

	SpriteOverlay m_spriteoverlay[4];

	void HandleChangeState();

	void EndGameTurn();

	void HandleAI(const uint8_t civindex);
	void AIRandomMove(const uint32_t unitindex, const int generaldirection, const bool forcemove, const uint64_t extrarandom);
	void AIMoveDirection(const uint32_t unitindex, const int direction);

	void AISettlerUnit(const uint32_t unitindex);
	void AIMilitaryLandUnit(const uint32_t unitindex);
	void AIMilitaryWaterUnit(const uint32_t unitindex);

	int32_t ClosestEnemyUnit(const uint8_t civindex, const int32_t x, const int32_t y, const bool musthavepath) const;
	int32_t ClosestFriendlyUnit(const uint8_t civindex, const int32_t x, const int32_t y) const;
	int32_t ClosestEnemyCity(const uint8_t civindex, const int32_t x, const int32_t y, const bool musthavepath) const;
	int32_t ClosestFriendlyCity(const uint8_t civindex, const int32_t x, const int32_t y) const;

	int32_t Distance2(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2) const;
	int Direction(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2) const;

	bool BaseTerrainInRadius(const int32_t x, const int32_t y, const int32_t r, const BaseTerrain::TerrainType terrain) const;

	void CheckSentry();	// goes through eveny unit and checks if enemy is in range and removed sentry if set

};
