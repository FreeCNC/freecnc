#ifndef _SCREENS_MAINMENU_H
#define _SCREENS_MAINMENU_H

#include "../freecnc.h"

class MainMenuScreen : public GameScreen
{
public:
    MainMenuScreen();
    ~MainMenuScreen();
    void run();
};

#endif
