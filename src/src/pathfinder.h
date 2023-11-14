#pragma once

#include <stdint.h>
#include "map.h"

class Pathfinder
{
public:
    Pathfinder();
    ~Pathfinder();

    void SetMap(Map *map);

    void InitializePathfinding();

    bool DirectConnection(int32_t x1, int32_t y1, const int32_t x2, const int32_t y2) const;
    bool Pathfind(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2, uint8_t &dir) const;

private:
    uint8_t *m_nodes;
    Map *m_map;
    int32_t m_mapwidth;
    int32_t m_mapheight;

    bool ClosestNode(const int32_t sx, const int32_t sy, int32_t &nodex, int32_t &nodey) const;

    bool ExpandNode(const int32_t node, const int32_t cost, uint8_t *openlist, uint8_t *dist, uint8_t *origdir) const;
    bool OnOpenList(const int32_t node, uint8_t *openlist) const;
    void OpenNode(const int32_t node, uint8_t *openlist) const;
    void CloseNode(const int32_t node, uint8_t *openlist) const;
    int32_t NextOpenNode(uint8_t *openlist, uint8_t *dist) const;

    int Direction(const int32_t x1, const int32_t y1, const int32_t x2, const int32_t y2) const;

};
