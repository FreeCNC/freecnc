#ifndef _GAME_GAME_H
#define _GAME_GAME_H

#include "../freecnc.h"

class Game
{
public:
    Game();
    ~Game();
    void play();
    void dumpstats();
    class GameError {};
private:
    ConfigType config;
    unsigned char gamemode;
};

#endif
