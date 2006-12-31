#include "mainmenu.h"
#include "playing.h"

MainMenuScreen::MainMenuScreen()
{
}

MainMenuScreen::~MainMenuScreen()
{
}

void MainMenuScreen::run()
{
    // Does nothing for the moment
    game.setscreen(new PlayingScreen());
}
