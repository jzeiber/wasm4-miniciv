#include "global.h"

namespace global
{
    int64_t ticks=0;
    SimplexNoise *noise=nullptr;
    Input *input=nullptr;
    Game *game=nullptr;

    void SetupGlobals()
    {
        ticks=0;
        noise=new SimplexNoise();
        input=new Input();
        game=new Game();
    }
}
