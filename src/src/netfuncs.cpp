#include "netfuncs.h"

#include "wasm4.h"

bool netplay_active()
{
    return (((*NETPLAY) & 0b100) == 0b100);
}

int8_t netplay_playeridx()
{
    if(netplay_active())
    {
        return ((*NETPLAY) & 0b11);
    }
    return -1;
}
