#ifndef _SCREENS_PLAYING_H
#define _SCREENS_PLAYING_H

#include "../freecnc.h"

class PlayingScreen : public GameScreen
{
public:
    PlayingScreen();
    ~PlayingScreen();
    void run();
};

#endif
