#include "playing.h"
#include "../game/game_public.h";

PlayingScreen::PlayingScreen()
{
}

PlayingScreen::~PlayingScreen()
{
}

void PlayingScreen::mainloop()
{
    game.log << "GameEngine: Entering main loop..." << endl;
    Game gsession;
    game.log << "GameEngine: Starting game..." << endl;
    gsession.play();
    game.setscreen();
}
