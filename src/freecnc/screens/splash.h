#ifndef _SCREENS_SPLASH_H
#define _SCREENS_SPLASH_H

#include "../freecnc.h"

class SplashScreen : public GameScreen
{
public:
    SplashScreen();
    ~SplashScreen();
    void mainloop();
};

#endif
