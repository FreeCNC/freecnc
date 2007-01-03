#include "playing.h"
#include "../game/game_public.h"

PlayingScreen::PlayingScreen()
{
}

PlayingScreen::~PlayingScreen()
{
}

void PlayingScreen::run()
{
    Game gsession;
    gsession.play();
    game.setscreen();
}
