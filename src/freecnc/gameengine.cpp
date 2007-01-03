#include <ctime>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <boost/program_options.hpp>

#include "SDL.h"

#include "gameengine.h"
#include "renderer/renderer_public.h"
#include "sound/sound_public.h"
#include "screens/mainmenu.h"
#include "screens/splash.h"
#include "legacyvfs/vfs_public.h"

using std::exception;
using std::ostringstream;
using std::runtime_error;

namespace po = boost::program_options;

void GameEngine::run()
{
    try {
        while (screen != 0) {
            screen->run();
        }
    } catch (exception& e) {
        log << "GameEngine: Fatal error: " << e.what() << endl;
        throw;
    }
}

void GameEngine::reconfigure()
{
    // Initialise SDL
    log << "GameEngine: Initialising SDL..." << endl;   
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
        ostringstream temp;
        temp << "GameEngine: SDL init failed: " << SDL_GetError();
        throw runtime_error(temp.str());
    }
    
    // Initialise Video
    try {
        log << "GameEngine: Initialising graphics engine..." << endl;
        pc::gfxeng = new GraphicsEngine();
    } catch (GraphicsEngine::VideoError&) {
        ostringstream temp;
        temp << "GameEngine: Graphics engine init failed";
        throw runtime_error(temp.str());
    }

    // Initialise Sound
    try {
        log << "GameEngine: Initialising sound engine..." << endl;
        pc::sfxeng = new SoundEngine(config.nosound);
    } catch (exception& e) {
        ostringstream temp;
        temp << "GameEngine: Sound engine init failed: " << e.what();
        throw runtime_error(temp.str());
    }
}

void GameEngine::startup(int argc, char** argv)
{
    parse_options(argc, argv);

    string logfile = game.config.basedir + "/freecnc.log";
    log.open(logfile.c_str());
    
    log << "GameEngine: Bootstrapping engine..." << endl;
    
    try { 
        // Legacy VFS / logging / conf parsing
        VFS_PreInit(config.basedir.c_str());

        string legacy_logfile = game.config.basedir + "/freecnc-legacy.log";
        logger = new Logger(legacy_logfile.c_str(),0);

        VFS_Init(config.basedir.c_str());
        VFS_LoadGame(config.gametype);

        reconfigure();

        if (config.debug) {
            log << "GameEngine: Debugging mode enabled" << endl;
        } else {
            // Hide the cursor since we have our own.
            SDL_ShowCursor(0);
        }
        
        // Init the rand functions
        srand(static_cast<unsigned int>(time(0)));

        // Play the intro if requested
        if (config.play_intro) {
            setscreen(new SplashScreen());
        } else {
            setscreen(new MainMenuScreen());
        }
    } catch (exception& e) {
        log << "GameEngine: Startup error: " << e.what() << endl;
        throw;
    }
}

void GameEngine::shutdown()
{
    log << "GameEngine: Shutting down..." << endl;
    
    // Legacy
    delete pc::gfxeng;
    delete pc::sfxeng;
    delete logger;

    VFS_Destroy();
    SDL_Quit();

    log.flush();   
}

void GameEngine::parse_options(int argc, char** argv)
{
    po::options_description generic("Generic options");
    generic.add_options()
        ("help,h", "show this message")
        ("version,v", "print version")
        ("basedir", po::value<string>(&config.basedir)->default_value("."),
            "use this location to find the data files");

    po::options_description game("Game options");
    game.add_options()
        ("map", po::value<string>(&config.map)->default_value("SCG01EA"),
            "start on this mission")
        ("fullscreen", po::bool_switch(&config.fullscreen)->default_value(false),
            "start fullscreen")
        ("width,w", po::value<int>(&config.width)->default_value(640),
            "use this value for screen width")
        ("height,h", po::value<int>(&config.height)->default_value(480),
            "use this value for screen height")
        ("bpp", po::value<int>(&config.bpp)->default_value(16),
            "colour depth in bits per pixel");

    po::options_description config_only("Config only options");
    config_only.add_options()
        ("gametype", po::value<int>()->default_value(GAME_TD),
            "Specify if the game is TD or RA")
        ("play_intro", po::value<bool>(&config.play_intro)->default_value(true),
            "enable/disable the intro")
        ("scale_movies", po::value<bool>(&config.scale_movies)->default_value(true),
            "enable/disable the scaler in video playback")
        ("scaler_quality", po::value<int>(&config.scaler_quality),
            "change which algorithm to use to scale the video")
        ("scrollstep", po::value<int>(&config.scrollstep)->default_value(1),
            "how many pixels to scroll in one step")
        ("scrolltime", po::value<int>(&config.scrolltime)->default_value(5),
            "how many ticks after releasing a scrollkey before slowing down")
        ("maxscroll", po::value<int>(&config.maxscroll)->default_value(24),
            "maximum speed for scrolling");

    po::options_description debug("Debug options");
    debug.add_options()
        ("nosound", "disable sound")
        ("debug", "turn on various internal debugging features");

    po::options_description cmdline_options, config_file_options;

    cmdline_options.add(generic).add(game).add(debug);
    config_file_options.add(game).add(config_only).add(debug);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::ostringstream s;
        s << cmdline_options;
        throw GameOptionsUsageMessage(s.str());
    }

    const string config_path(config.basedir + "/data/settings/freecnc.cfg");
    std::ifstream cfgfile(config_path.c_str());

    po::store(po::parse_config_file(cfgfile, config_file_options), vm);
    po::notify(vm);

    // This is ugly, but not sure how permanent gametype will be.
    config.gametype = static_cast<GameType>(vm["gametype"].as<int>());

    // Values that are now no longer configurable
    config.buildable_ratio = 0.7;
    config.buildable_radius = 2;
    config.final_delay = 100;
}
