#include "wasm4.h"
#include "global.h"
#include "palette.h"

void start()
{
    PALETTE[0]=0x000000;
    PALETTE[1]=0x997344;
    PALETTE[2]=0x85ffff;
    PALETTE[3]=0xffffff;

    global::SetupGlobals();
}

void update()
{
    global::ticks++;

    global::input->Update();
    global::game->HandleInput(global::input,0);
    global::game->Update(1,0,global::game);
    global::game->Draw(0);
}
