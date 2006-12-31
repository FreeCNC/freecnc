#include <iostream>
#include <stdexcept>

#include "../renderer/renderer_public.h"
#include "../sound/sound_public.h"
#include "../ui/ui_public.h"
#include "actioneventqueue.h"
#include "dispatcher.h"
#include "game.h"
#include "map.h"
#include "playerpool.h"

using std::runtime_error;
using std::endl;

Game::Game() {
    string tmp;
    ConfigType legacy_config = getConfig();

    /// @TODO We've already loaded files.ini in the vfs.
    shared_ptr<INIFile> fileini = GetConfig("files.ini");

    INIKey key = fileini->readIndexedKeyValue("general", legacy_config.gamenum, "PLAY");

    /// @TODO This should throw on its own accord
    if (!pc::sfxeng->CreatePlaylist(key->second.c_str())) {
        throw runtime_error("Could not create playlist");
    }

    scoped_ptr<LoadingScreen> loadscreen(new LoadingScreen());

    loadscreen->setCurrentTask("Creating the ActionEventQueue");
    p::aequeue = new ActionEventQueue();

    loadscreen->setCurrentTask("Loading the map.");

    p::ccmap = new CnCMap();
    p::ccmap->loadMap(game.config.map.c_str(), loadscreen.get());

    p::dispatcher = new Dispatcher::Dispatcher();

    loadscreen.reset(0);

    try {
        VQAMovie mov(p::ccmap->getMissionData().brief);
        mov.play();
    } catch (runtime_error&) {
    }
    try {
        VQAMovie mov(p::ccmap->getMissionData().action);
        mov.play();
    } catch (runtime_error&) {
    }

    pc::sidebar = new Sidebar(p::ppool->getLPlayer(), pc::gfxeng->getHeight(),
            p::ccmap->getMissionData().theater);

    pc::cursor = new Cursor();

    pc::input = new Input(pc::gfxeng->getWidth(), pc::gfxeng->getHeight(),
                          pc::gfxeng->getMapArea());
}

Game::~Game()
{
    delete pc::input;
    delete pc::cursor;
    delete pc::sidebar;
    delete p::dispatcher;
    delete p::ccmap;
    delete p::aequeue;
}

/** Play the mission. */
void Game::play()
{
    pc::sfxeng->PlayTrack(p::ccmap->getMissionData().theme);
    pc::gfxeng->setupCurrentGame();

    // @TODO: Jump to correct start location in multiplayer games.
    p::ccmap->restoreLocation(0);

    while (!pc::input->shouldQuit()) {
        pc::gfxeng->renderScene();
        p::aequeue->runEvents();
        pc::input->handle();
    }
    pc::sfxeng->StopMusic();

    try {
        string mov_name;
        if (p::ppool->hasWon()) {
            mov_name = p::ccmap->getMissionData().winmov;
        } else {
            mov_name = p::ccmap->getMissionData().losemov;
        }
        VQAMovie mov(mov_name.c_str());
        mov.play();
    } catch (runtime_error&) {
    }

    dumpstats();
}

inline const char* plural(int value)
{
    if (value == 1) {
        return " ";
    }
    return "s ";
}

void log_player_stats(Player* pl)
{
    game.log << pl->getName() << "\n"
             << "\tUnit kills:       " << pl->getUnitKills() << "\n"
             << "\t     losses:      " << pl->getUnitLosses() << "\n"
             << "\tStructure kills:  " << pl->getStructureKills() << "\n"
             << "\t          losses: " << pl->getStructureLosses() << endl;
}

void Game::dumpstats()
{
    unsigned int uptime = p::aequeue->getElapsedTime() / 1000;

    int hours = uptime / 3600;
    uptime %= 3600;
    int minutes = uptime / 60;
    int seconds = uptime % 60;

    logger->renderGameMsg(false);
    game.log << "Time wasted: " << hours << " hour" << plural(hours)
             << minutes << " minute" << plural(minutes)
             << seconds << " second" << plural(seconds) << endl;

    /// @TODO Make PlayerPool return iterators and make this for_each
    for (int i = 0; i < p::ppool->getNumPlayers(); i++) {
        Player* pl = p::ppool->getPlayer(i);
        log_player_stats(pl);
    }
}
