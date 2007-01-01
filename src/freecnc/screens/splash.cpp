#include "splash.h"
#include "mainmenu.h"
#include "../renderer/renderer_public.h"

SplashScreen::SplashScreen()
{
}

SplashScreen::~SplashScreen()
{
}

void SplashScreen::run()
{
    try {
        VQAMovie mov(game.config.gametype != GAME_RA ? "logo" : "prolog");
        mov.play();
    } catch (std::runtime_error& e) {
    }
    //WSA choose("choose.wsa");
    //choose.animate();
    
    game.setscreen(new MainMenuScreen());
}
