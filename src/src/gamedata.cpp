#include "gamedata.h"
#include "game.h"
#include "cppfuncs.h"
#include "randommt.h"
#include "unitdata.h"
#include "wasmstring.h"
#include "wasm4.h"

GameData::GameData():m_map(nullptr),m_seed(0),m_ticks(0),m_gamestarted(false),m_gameturn(0),m_currentcivturn(0),m_civplayernum{0,0,0,0},m_playerlastactivity{0,0,0,0},m_playeractive{false,false,false,false},m_playerready{false,false,false,false}
{
    m_map=new Map();
    m_pathfinder=new Pathfinder();
}

GameData::~GameData()
{
    // gamedata doesn't get destroyed over the life of the program, so we don't need to delete map and pathfinder and save some program size
}

void GameData::SetupNewGame(const uint64_t seed)
{
    // setup new game vars

    m_seed=seed;
    m_map->SetSeed(seed);
    //m_map->SetSize(128,96);
    m_map->SetSize(80,48);
    m_pathfinder->SetMap(m_map);        // must set pathfinder map after map size is set
    m_pathfinder->InitializePathfinding();

    m_gameturn=1;
    m_currentcivturn=0;
    m_turnstarttick=m_ticks;
    m_turntimelimit=(4*60*60);   // 4 minutes to start with

    for(size_t i=0; i<countof(m_civplayernum); i++)
    {
        m_civplayernum[i]=0;
        m_playerready[i]=false;
    }

    // find 4 land map coords to place initial settlers
    RandomMT rand(m_seed);
    MapCoord mc[MAX_CIVILIZATIONS]={MapCoord(m_map->Width(),m_map->Height(),0,0),MapCoord(m_map->Width(),m_map->Height(),0,0),MapCoord(m_map->Width(),m_map->Height(),0,0),MapCoord(m_map->Width(),m_map->Height(),0,0)};
    for(size_t i=0; i<countof(m_civ); i++)
    {
        mc[i].Set((m_map->Width()/4)+((i%2)*(m_map->Width()/2)),(m_map->Height()/4)+((i/2)*(m_map->Height()/2)));

        bool good=false;
        int32_t rad=0;
        while(good==false && rad<m_map->Width())
        {
            for(int dy=-rad; dy<=rad && good==false; dy++)
            {
                for(int dx=-rad; dx<=rad && good==false; dx++)
                {
                    TerrainTile t=m_map->GetTile(mc[i].X()+dx,mc[i].Y()+dy);
                    if(t.BaseType()==BaseTerrain::BASETERRAIN_LAND)
                    {
                        good=true;
                        mc[i].Set(mc[i].X()+dx,mc[i].Y()+dy);
                    }
                }
            }
            rad++;
        }
    }
    // shuffle start locations so they're not in same civ order each game
    for(size_t i=0; i<countof(m_civ); i++)
    {
        // swap this idx with rand idx
        int8_t r=rand.Next()%4;
        if(r!=i)    // don't swap with itself
        {
            MapCoord temp=mc[i];
            mc[i]=mc[r];
            mc[r]=temp;
        }
    }
    // create initial settler unit for each civ
    for(size_t i=0; i<countof(m_civ); i++)
    {
        m_civ[i].gold=10;
        //m_civ[i].gold=5000;

        size_t idx=(i*CITIES_PER_CIVILIZATION*UNITS_PER_CITY);
        m_unit[idx].flags=UNIT_ALIVE;
        m_unit[idx].owner=i;
        m_unit[idx].movesleft=unitdata[UNITTYPE_SETTLER].moves;
        m_unit[idx].type=UNITTYPE_SETTLER;
        m_unit[idx].x=mc[i].X();
        m_unit[idx].y=mc[i].Y();

        /*
        // debug - also create militia
        m_unit[idx+1].flags=UNIT_ALIVE | UNIT_VETERAN;
        m_unit[idx+1].owner=i;
        m_unit[idx+1].movesleft=unitdata[UNITTYPE_MILITIA].moves;
        m_unit[idx+1].type=UNITTYPE_MILITIA;
        m_unit[idx+1].x=mc[i].X();
        m_unit[idx+1].y=mc[i].Y();

        // debug - also create trireme
        int32_t tidx=idx+2;
        m_unit[idx+2].flags=UNIT_ALIVE | UNIT_VETERAN;
        m_unit[idx+2].owner=i;
        m_unit[idx+2].movesleft=unitdata[UNITTYPE_TRIREME].moves;
        m_unit[idx+2].type=UNITTYPE_TRIREME;
        m_unit[idx+2].x=mc[i].X();
        m_unit[idx+2].y=mc[i].Y();

        m_unit[idx+3].flags=UNIT_ALIVE;
        m_unit[idx+3].owner=i;
        m_unit[idx+3].movesleft=unitdata[UNITTYPE_TRIREME].moves;
        m_unit[idx+3].type=UNITTYPE_TRIREME;
        m_unit[idx+3].x=mc[i].X();
        m_unit[idx+3].y=mc[i].Y();

        idx=(i*CITIES_PER_CIVILIZATION);
        //for(idx=(i*CITIES_PER_CIVILIZATION); idx<(i*CITIES_PER_CIVILIZATION)+CITIES_PER_CIVILIZATION; idx++)
        {
            m_city[idx].owner=i;
            m_city[idx].population=3;
            m_city[idx].food=0;
            m_city[idx].improvements=(0x01 << IMPROVEMENT_CITYWALLS) | (0x01 << IMPROVEMENT_BANK);
            m_city[idx].producing=0;
            m_city[idx].shields=0;
            m_city[idx].x=mc[i].X();
            m_city[idx].y=mc[i].Y();
            for(int32_t dy=-10; dy<11; dy++)
            {
                for(int32_t dx=-10; dx<11; dx++)
                {
                    if(m_map->GetBaseType(dx+mc[i].X(),dy+mc[i].Y())==BaseTerrain::BASETERRAIN_LAND && (m_map->GetBaseType(dx+mc[i].X()-1,dy+mc[i].Y())==BaseTerrain::BASETERRAIN_WATER || m_map->GetBaseType(dx+mc[i].X()+1,dy+mc[i].Y())==BaseTerrain::BASETERRAIN_WATER))
                    {
                        MapCoord mcty(m_map->Width(),m_map->Height(),dx+mc[i].X(),dy+mc[i].Y());
                        m_city[idx].x=mcty.X();
                        m_city[idx].y=mcty.Y();
                        // also set trireme idx
                        m_unit[tidx].x=mcty.X();
                        m_unit[tidx].y=mcty.Y();
                        m_unit[tidx+1].x=mcty.X();
                        m_unit[tidx+1].y=mcty.Y();
                        break;
                    }
                }
            }

        }
        */
    }

}

void GameData::SaveGame()
{
    char *buff=new char[1024];
    memset(buff,0,1024);

    uint32_t pos=0;
    uint32_t magic=0x01020006;
    memcpy(&buff[pos],&magic,4);
    pos+=4;

    memcpy(&buff[pos],&m_seed,sizeof(m_seed));
    pos+=sizeof(m_seed);
    memcpy(&buff[pos],&m_ticks,sizeof(m_ticks));
    pos+=sizeof(m_ticks);
    memcpy(&buff[pos],&m_gameturn,sizeof(m_gameturn));
    pos+=sizeof(m_gameturn);
    memcpy(&buff[pos],&m_currentcivturn,sizeof(m_currentcivturn));
    pos+=sizeof(m_currentcivturn);
    memcpy(&buff[pos],&m_turntimelimit,sizeof(m_turntimelimit));
    pos+=sizeof(m_turntimelimit);

    memcpy(&buff[pos],&m_civ,sizeof(Civilization)*MAX_CIVILIZATIONS);
    pos+=sizeof(Civilization)*MAX_CIVILIZATIONS;

    memcpy(&buff[pos],&m_city,sizeof(City)*MAX_CITIES);
    pos+=sizeof(City)*MAX_CITIES;

    memcpy(&buff[pos],&m_unit,sizeof(Unit)*MAX_UNITS);
    pos+=sizeof(Unit)*MAX_UNITS;

    /*
    //debug - print out save game data
    {
        OutputStringStream ostr;
        ostr << "Save Size = " << pos << "  seed=" << m_seed;
        trace(ostr.Buffer());

        
        ostr.Clear();
        for(size_t i=0; i<1024; i++)
        {
            ostr << (int32_t)buff[i] << ",";
            if((i+1)%32==0)
            {
                trace(ostr.Buffer());
                ostr.Clear();
            }
        }
        trace(ostr.Buffer());   
    }
    */

    diskw(buff,1024);
    delete [] buff;
}

bool GameData::LoadGame()
{
    bool loaded=false;
    char *buff=new char[1024];
    uint32_t len=diskr(buff,1024);

    if(len==1024)
    {
        uint32_t pos=0;
        uint32_t magic=0;
        memcpy(&magic,&buff[pos],4);
        pos+=4;

        if(magic==0x01020006)
        {
            memcpy(&m_seed,&buff[pos],sizeof(m_seed));
            pos+=sizeof(m_seed);
            memcpy(&m_ticks,&buff[pos],sizeof(m_ticks));
            pos+=sizeof(m_ticks);
            memcpy(&m_gameturn,&buff[pos],sizeof(m_gameturn));
            pos+=sizeof(m_gameturn);
            memcpy(&m_currentcivturn,&buff[pos],sizeof(m_currentcivturn));
            pos+=sizeof(m_currentcivturn);
            memcpy(&m_turntimelimit,&buff[pos],sizeof(m_turntimelimit));
            pos+=sizeof(m_turntimelimit);

            memcpy(&m_civ,&buff[pos],sizeof(Civilization)*MAX_CIVILIZATIONS);
            pos+=sizeof(Civilization)*MAX_CIVILIZATIONS;

            memcpy(&m_city,&buff[pos],sizeof(City)*MAX_CITIES);
            pos+=sizeof(City)*MAX_CITIES;

            memcpy(&m_unit,&buff[pos],sizeof(Unit)*MAX_UNITS);
            pos+=sizeof(Unit)*MAX_UNITS;

            // setup other vars that aren't saved
            m_map->SetSeed(m_seed);
            //m_map->SetSize(128,96);
            m_map->SetSize(80,48);
            m_pathfinder->SetMap(m_map);        // must set pathfinder map after map size is set
            m_pathfinder->InitializePathfinding();

            m_turnstarttick=m_ticks;

            for(size_t i=0; i<countof(m_civplayernum); i++)
            {
                m_civplayernum[i]=0;
                m_playerready[i]=false;
            }

            /*
            //debug
            {
                OutputStringStream ostr;
                ostr << "Loaded seed=" << m_seed;
                trace(ostr.Buffer());
            }
            */

            loaded=true;
        }
    }

    delete [] buff;
    return loaded;
}

int8_t GameData::GetCivIndexFromPlayerNum(const uint8_t playernum) const
{
    for(size_t i=0; i<countof(m_civplayernum); i++)
    {
        if(m_civplayernum[i]==playernum)
        {
            return i;
        }
    }
    return -1;
}

int8_t GameData::GetNextFreeCivIndex(const int8_t startnum, const int8_t dir) const
{
    int8_t n=startnum < 0 ? 0 : startnum;
    for(int i=0; i<countof(m_civplayernum); i++,n+=dir)
    {
        if(n>=static_cast<int8_t>(countof(m_civplayernum)))
        {
            n=0;
        }
        else if(n<0)
        {
            n=countof(m_civplayernum)-1;
        }
        if(m_civplayernum[n]==0)
        {
            return n;
        }
    }
    return -1;
}

void GameData::AssignPlayerNumCivIndex(const uint8_t playernum, const int8_t civindex)
{
    if(civindex>=0 && civindex<countof(m_civplayernum))
    {
        for(size_t i=0; i<countof(m_civplayernum); i++)
        {
            if(i==civindex) // assign player to this index
            {
                m_civplayernum[i]=playernum;
            }
            else if(m_civplayernum[i]==playernum)   // make sure player gets cleared from any other index
            {
                m_civplayernum[i]=0;
            }
        }
    }
}

void GameData::ClearPlayerNumCivIndex(const uint8_t playernum)
{
    for(size_t i=0; i<countof(m_civplayernum); i++)
    {
        if(m_civplayernum[i]==playernum)
        {
            m_civplayernum[i]=0;
        }
    }
}

bool GameData::AllPlayersReady() const
{
    bool ready=true;

    for(size_t i=0; i<countof(m_playerlastactivity); i++)
    {
        if(m_playerlastactivity[i]>0 && m_playerlastactivity[i]+PLAYER_TIMEOUT>m_ticks && m_playerready[i]==false)
        {
            ready=false;
        }
    }
    return ready;
}
