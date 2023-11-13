#include "pathfinder.h"
#include "wasmstring.h"
#include "global.h"

Pathfinder::Pathfinder():m_nodes(nullptr),m_map(nullptr)
{
    m_nodes=new uint8_t[256];
}

Pathfinder::~Pathfinder()
{
    if(m_nodes)
    {
        delete [] m_nodes;
    }
}

void Pathfinder::SetMap(Map *map)
{
    m_map=map;
    if(m_map)
    {
        m_mapwidth=m_map->Width();
        m_mapheight=m_map->Height();
    }
    else
    {
        m_mapwidth=0;
        m_mapheight=0;
    }
}

void Pathfinder::InitializePathfinding()
{
    if(m_map)
    {
        int32_t node=0;
        for(int32_t y=3; y<128; y+=8)
        {
            for(int32_t x=3; x<128; x+=8,node++)
            {
                const BaseTerrain::TerrainType terr=m_map->GetBaseType(x,y);
                m_nodes[node]=0;
                int8_t bit=0;
                // try to find path of same terrain to each of the 8 connections of surrounding nodes
                for(int32_t dy=-8; dy<9; dy+=8)
                {
                    for(int32_t dx=-8; dx<9; dx+=8)
                    {
                        if(dx!=0 || dy!=0)
                        {
                            //if(NodesConnected(x,y,dx,dy,terr)==true)
                            if(DirectConnection(x,y,dx+x,dy+y)==true)
                            {
                                m_nodes[node]|=(0x01 << bit);
                            }
                            bit++;
                        }
                    }
                }
            }
        }
    }
}
/*
bool Pathfinder::NodesConnected(const int32_t x, const int32_t y, const int32_t dx, const int32_t dy, const BaseTerrain::TerrainType terrain) const
{
    MapCoord mc(m_map->Width(),m_map->Height(),0,0);
    const int32_t ddx=(dx<0 ? -1 : (dx>0 ? 1 : 0));
    const int32_t ddy=(dy<0 ? -1 : (dy>0 ? 1 : 0));
    int32_t xx=x+ddx;
    int32_t yy=y+ddy;

    // we can start at +ddx,+ddy because we already have the terrain of the source x,y
    for(int32_t cnt=0; cnt<8; cnt++,xx+=ddx,yy+=ddy)
    {
        mc.Set(xx,yy);
        if(m_map->GetBaseType(mc.X(),mc.Y())!=terrain)
        {
            return false;
        }
    }
    return true;
}
*/
bool Pathfinder::DirectConnection(int32_t x1, int32_t y1, const int32_t x2, const int32_t y2) const
{
    MapCoord mc(m_mapwidth,m_mapheight,x1,y1);
    const int32_t dx=(x2-x1)<0 ? x1-x2 : x2-x1;
    const int32_t sx=x1<x2 ? 1 : -1;
    const int32_t dy=(y2-y1)>0 ? y1-y2 : y2-y1;
    const int32_t sy=y1<y2 ? 1 : -1;
    const int32_t error=dx+dy;
    int32_t e2=0;
    int32_t err=0;

    const BaseTerrain::TerrainType terr=m_map->GetBaseType(mc.X(),mc.Y());

    while(m_map->GetBaseType(mc.X(),mc.Y())==terr)
    {
        if(x1==x2 && y1==y2)
        {
            return true;
        }
        e2=2*err;
        if(e2>=dy && x1!=x2)
        {
            err=err+dy;
            x1=x1+sx;
        }
        if(e2<=dx && y1!=y2)
        {
            err=err+dx;
            y1=y1+sy;
        }
        mc.Set(x1,y1);
    }

    return false;
}

bool Pathfinder::Pathfind(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2, uint8_t &dir) const
{
    MapCoord mc(m_mapwidth,m_mapheight,x1,y1);
    BaseTerrain::TerrainType sourceterr=m_map->GetBaseType(mc.X(),mc.Y());
    mc.Set(x2,y2);
    BaseTerrain::TerrainType destterr=m_map->GetBaseType(mc.X(),mc.Y());

    if(sourceterr!=destterr)
    {
        return false;
    }

    int32_t n1x;
    int32_t n1y;
    int32_t n2x;
    int32_t n2y;

    if(ClosestNode(x1,y1,n1x,n1y) && ClosestNode(x2,y2,n2x,n2y))
    {
        // do this first so we don't have to allocate memory and return quickly if source and dest are close
        const int32_t destidx=(((n2y-3)/8)*16)+((n2x-3)/8);
        int32_t nidx=(((n1y-3)/8)*16)+((n1x-3)/8);

        // if start node and destinatino node are the same, then just return direction to destination
        if(destidx==nidx)
        {
            // set destination dir
            dir=Direction(x1,y1,x2,y2);
            return true;
        }

        uint8_t *dist=new uint8_t[256];     // distance to each node (# hops from source node)
        uint8_t *origdir=new uint8_t[256];  // hold dir from original node that replicates as nodes are expanded - used to find which path resulted in reaching destination
        uint8_t *openlist=new uint8_t[32];  // bitlist of which nodes are open waiting to expand

        memset(dist,255,256);
        memset(origdir,0,256);
        memset(openlist,0,32);

        dist[nidx]=0;
        origdir[nidx]=255;

        int32_t cnt=0;
        do
        {
            ExpandNode(nidx,dist[nidx],openlist,dist,origdir);
            nidx=NextOpenNode(openlist,dist);
        } while (nidx>=0 && nidx!=destidx && cnt++<200);

        // we found the destination
        // backtrack to find direction to destination
        if(nidx==destidx)
        {
            // origdir for dest
            dir=origdir[destidx];
        }

        delete [] dist;
        delete [] origdir;
        delete [] openlist;

        if(nidx==destidx)
        {
            return true;
        }

    }

    return false;
}

bool Pathfinder::ClosestNode(const int32_t sx, const int32_t sy, int32_t &nodex, int32_t &nodey) const
{
    MapCoord mc(m_mapwidth,m_mapheight,0,0);

    int32_t nodepos[8]; // x,y pairs of node positions around sx,sy
    int8_t nodedist[4]={-1,-1,-1,-1};
    int8_t closestnode=-1;
    int8_t closestdist=-1;

    // wrap x coord to the right for the top left x coord (so negative x is handled)
    int32_t x=sx;
    if(x<3)
    {
        x+=m_mapwidth;
    }

    // top left
    nodepos[0]=(((x-3)/8)*8)+3;
    nodepos[1]=(((sy-3)/8)*8)+3;

    // top right
    nodepos[2]=nodepos[0]+8;
    nodepos[3]=nodepos[1];

    // bottom left
    nodepos[4]=nodepos[0];
    nodepos[5]=nodepos[1]+8;

    // bottom right
    nodepos[6]=nodepos[0]+8;
    nodepos[7]=nodepos[1]+8;

    for(size_t i=0; i<4; i++)
    {
        if(DirectConnection(x,sy,nodepos[i*2],nodepos[(i*2)+1])==true)
        {
            const int32_t dx=x<nodepos[i*2] ? nodepos[i*2]-x : x-nodepos[i*2];
            const int32_t dy=sy<nodepos[(i*2)+1] ? nodepos[(i*2)+1]-sy : sy-nodepos[(i*2)+1];
            const int32_t dist=dx>dy ? dx : dy;
            if(closestnode==-1 || dist<closestdist)
            {
                closestnode=i;
                closestdist=dist;
            }
        }
    }

    if(closestnode>=0)
    {
        mc.Set(nodepos[closestnode*2],nodepos[(closestnode*2)+1]);
        nodex=mc.X();
        nodey=mc.Y();
    }
    return closestnode>=0;
}

bool Pathfinder::ExpandNode(const int32_t node, const int32_t cost, uint8_t *openlist, uint8_t *dist, uint8_t *origdir) const
{
    CloseNode(node,openlist);

    // we already had expanded this node at a previously lower cost
    if(dist[node]<cost)
    {
        return false;
    }

    dist[node]=cost;

    // expand to all the surrounding connected nodes
    int8_t bit=0;
    for(int32_t ndy=-1; ndy<2; ndy++)
    {
        for(int32_t ndx=-1; ndx<2; ndx++)
        {
            if(ndx!=0 || ndy!=0)
            {
                // must wrap xpos on same line, so need to get x coord /16*16 and then add modulus of addition with offset
                const int32_t onode=(ndy*16)+((node/16)*16)+((ndx+node)%16);  // surrounding node node index
                if(onode>=0 && onode<256 && ((m_nodes[node] & (0x01 << bit)) == (0x01 << bit)) && (cost+1)<dist[onode])
                {
                    dist[onode]=cost+1;
                    origdir[onode]=origdir[node]!=255 ? origdir[node] : (0x01 << bit);
                    OpenNode(onode,openlist);
                }
                bit++;
            }
        }
    }

    return true;
}

bool Pathfinder::OnOpenList(const int32_t node, uint8_t *openlist) const
{
    if(node>=0 && node<256)
    {
        int32_t byte=node/8;
        int32_t bit=node%8;
        return ((openlist[byte] & (0x01 << bit)) == (0x01 << bit));
    }
    return false;
}

void Pathfinder::OpenNode(const int32_t node, uint8_t *openlist) const
{
    if(node>=0 && node<256)
    {
        int32_t byte=node/8;
        int32_t bit=node%8;
        openlist[byte]|=(0x01 << bit);
    }
}

void Pathfinder::CloseNode(const int32_t node, uint8_t *openlist) const
{
    if(node>=0 && node<256)
    {
        int32_t byte=node/8;
        int32_t bit=node%8;
        openlist[byte]=(openlist[byte] & ~(0x01 << bit));
    }
}

int32_t Pathfinder::NextOpenNode(uint8_t *openlist, uint8_t *dist) const
{
    // find next open node with lowest cost
    int32_t bestnode=-1;
    for(size_t i=0; i<256; i++)
    {
        if(OnOpenList(i,openlist) && (bestnode==-1 || dist[i]<dist[bestnode]))
        {
            bestnode=i;
        }
    }
    return bestnode;
}

int Pathfinder::Direction(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2) const
{
	MapCoord mc(m_mapwidth,m_mapheight,0,0);

	int32_t dy=y2-y1;
	int32_t dx=(x2-x1);
	if(dx<0)
	{
		dx=dx<-(m_mapwidth/2) ? -(m_mapwidth+dx) : dx;
	}
	else
	{
		dx=dx>(m_mapwidth/2) ? m_mapwidth-dx : dx;
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