#pragma once

#include <stdint.h>

enum IconType
{
    ICON_NONE=0,
    ICON_NEXTUNIT=1,
    ICON_NEXTLOCUNIT=2,
    ICON_NEXTCITY=3,
    ICON_SCROLLMAP=4,
    ICON_TOGGLEINFO=5,
    ICON_ENDTURN=6,
    ICON_VIEWMAP=7,
    ICON_FOUNDCITY=8,
    ICON_CLOSE=9,
    ICON_CIVDATA=10,
    ICON_ENTERCITY=11,
    ICON_DISBAND=12,
    ICON_EXPANDCITY=13,
    ICON_CHANGEBUILD=14,
    ICON_BUYBUILD=15
};

struct IconData
{
    const char *name;
    uint8_t xidx;
    uint8_t yidx;
};

extern const IconData icondata[];