#include <stdexcept>

#include "../renderer/renderer_public.h"
#include "../sound/sound_public.h"
#include "../ui/ui_public.h"
#include "actioneventqueue.h"
#include "dispatcher.h"
#include "game.h"
#include "map.h"
#include "playerpool.h"

/** Constructor, loads the map, sidebar and such. plays briefing and actionmovie
 */
Game::Game() {
    string tmp;
    ConfigType config = getConfig();
    /// @TODO We've already loaded files.ini in the vfs.
    shared_ptr<INIFile> fileini = GetConfig("files.ini");
    INIKey key = fileini->readIndexedKeyValue("general", config.gamenum, "PLAY");
    if (!pc::sfxeng->CreatePlaylist(key->second.c_str())) {
        logger->error("Could not create playlist!\n");
        throw GameError();
    }
    LoadingScreen* loadscreen(new LoadingScreen());
    gamemode = config.gamemode;

    /* reset the tickcounter, should be a function in the class conatining the
     * counter */
    loadscreen->setCurrentTask("Creating the ActionEventQueue");
    p::aequeue = new ActionEventQueue();
    /* load the map */
    loadscreen->setCurrentTask("Loading the map.");
    try {
        p::ccmap = new CnCMap();
        p::ccmap->loadMap(config.mapname.c_str(), loadscreen);
    } catch (CnCMap::LoadMapError&) {
        delete loadscreen;
        // loadmap will have printed the error
        throw GameError();
    }
    p::dispatcher = new Dispatcher::Dispatcher();
    switch (config.dispatch_mode) {
        case 0:
            break;
        case 1:
            // Record
            break;
        case 2:
            // Playback
            break;
        default:
            logger->error("Invalid dispatch mode: %i\n",config.dispatch_mode);
            throw GameError();
            break;
    }

    delete loadscreen;
    switch (gamemode) {
    case 0:
        try {
            VQAMovie mov(p::ccmap->getMissionData().brief);
            mov.play();
        } catch (std::runtime_error&) {
        }
        try {
            VQAMovie mov(p::ccmap->getMissionData().action);
            mov.play();
        } catch (std::runtime_error&) {
        }
        break;
    case 1:
    case 2:
    default:
        break;
    }
    /* init sidebar */
    try {
        pc::sidebar = new Sidebar(p::ppool->getLPlayer(), pc::gfxeng->getHeight(),
                p::ccmap->getMissionData().theater);
    } catch (Sidebar::SidebarError&) {
        throw GameError();
    }
    /* init cursor */
    pc::cursor = new Cursor();
    /* init the input functions */
    pc::input = new Input(pc::gfxeng->getWidth(), pc::gfxeng->getHeight(),
                          pc::gfxeng->getMapArea());
}

/** Destructor, frees up some memory */
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
    // Start the music
    pc::sfxeng->PlayTrack(p::ccmap->getMissionData().theme);
    pc::gfxeng->setupCurrentGame();
    // Jump to start location
    // @TODO: Jump to correct start location in multiplayer games.
    p::ccmap->restoreLocation(0);
    /* main gameloop */
    while (!pc::input->shouldQuit()) {
        // Draw the scene
        pc::gfxeng->renderScene();
        // Run scheduled events
        p::aequeue->runEvents();
        // Handle the input
        pc::input->handle();

        if (gamemode == 2) {
            // Synchronise events with server
        }
    }
    // Stop the music
    pc::sfxeng->StopMusic();

    if (gamemode == 0) {
        if (p::ppool->hasWon() ) {
            try {
                scoped_ptr<VQAMovie> mov(new VQAMovie(p::ccmap->getMissionData().winmov));
                mov->play();
            } catch (std::runtime_error&) {
            }
        } else if (p::ppool->hasLost() ) {
            try {
                scoped_ptr<VQAMovie> mov(new VQAMovie(p::ccmap->getMissionData().losemov));
                mov->play();
            } catch (std::runtime_error&) {
            }
        }
    }
    dumpstats();
}

void Game::dumpstats()
{
    Player* pl;
    Uint8 h,m,s,i;
    Uint32 uptime = p::aequeue->getElapsedTime();
    uptime /= 1000;
    h = uptime/3600;
    uptime %= 3600;
    m = uptime/60;
    s = uptime%60;
    logger->renderGameMsg(false);
    logger->gameMsg("Time wasted: %i hour%s%i minute%s%i second%s",
                    h,(h!=1?"s ":" "),m,(m!=1?"s ":" "),s,(s!=1?"s ":" "));
    for (i = 0; i < p::ppool->getNumPlayers(); i++) {
        pl = p::ppool->getPlayer(i);
        logger->gameMsg("%s\nUnit kills:  %i\n     losses: %i\n"
                        "Structure kills:  %i\n          losses: %i\n",
                        pl->getName(),pl->getUnitKills(),pl->getUnitLosses(),
                        pl->getStructureKills(),pl->getStructureLosses());
    }
}
