#pragma once

#include <stdint.h>

enum IconType
{
    ICON_NONE=0,
    ICON_NEXTUNIT=1,
    ICON_NEXTCITY=2,
    ICON_SCROLLMAP=3,
    ICON_TOGGLEINFO=4,
    ICON_ENDTURN=5,
    ICON_VIEWMAP=6,
    ICON_FOUNDCITY=7,
    ICON_CLOSE=8,
    ICON_CIVDATA=9,
    ICON_ENTERCITY=10,
    ICON_DISBAND=11,
    ICON_EXPANDCITY=12,
    ICON_CHANGEBUILD=13,
    ICON_BUYBUILD=14
};

struct IconData
{
    const char *name;
    uint8_t xidx;
    uint8_t yidx;
};

extern const IconData icondata[];