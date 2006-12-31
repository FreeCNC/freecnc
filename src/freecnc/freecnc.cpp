//
// Entry point, engine initialization, cleanup
//
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include <boost/program_options.hpp>

#include "SDL.h"

#include "freecnc.h"
#include "lib/inifile.h"
#include "game/game_public.h"
#include "renderer/renderer_public.h"
#include "sound/sound_public.h"
#include "vfs/vfs_public.h"

namespace po = boost::program_options;

using std::cout;
using std::cerr;
using std::exception;
using std::ofstream;
using std::ostringstream;
using std::runtime_error;

#if defined _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sstream>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

GameEngine game;

class UsageMessage : public exception
{
public:
    UsageMessage(const string& message) : message(message) {}
    ~UsageMessage() throw() {}
    const char* what() const throw() {return message.c_str();}
private:
    string message;
};

// Legacy
Logger *logger;
bool parse(int argc, char** argv);

int main(int argc, char** argv)
{
    try {
        game.startup(argc, argv);
        game.mainloop();
    } catch (UsageMessage& usage) {
        #if _WIN32
        MessageBox(0, usage.what(), "FreeCNC command line options", MB_ICONINFORMATION|MB_OK);
        #else
        cout << usage.what() << endl;
        #endif
        return 1;
    } catch (exception& e) {
        #if _WIN32
        MessageBox(0, e.what(), "Fatal error - FreeCNC", MB_ICONERROR|MB_OK);
        #else
        cerr << "Fatal Error: " << e.what() << endl;
        #endif
        return 1;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// GameEngine
//-----------------------------------------------------------------------------

void GameEngine::mainloop()
{
    try {
        log << "GameEngine: Entering main loop..." << endl;
        Game gsession;
        log << "GameEngine: Starting game..." << endl;
        gsession.play();
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
        if (!parse(argc, argv)) {
            throw runtime_error("GameEngine: parse parsing error");
        }

        const ConfigType& legacy_config = getConfig();
        VFS_Init(config.basedir.c_str());
        VFS_LoadGame(legacy_config.gamenum);

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
            log << "GameEngine: Playing intro..." << endl;
            try {
                VQAMovie mov(legacy_config.gamenum != GAME_RA ? "logo" : "prolog");
                mov.play();
            } catch (runtime_error&) {
            }

            try {
                WSA choose("choose.wsa");
                choose.animate();
            } catch (WSA::WSAError&) {
            }
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
            "use this value for screen height");

    po::options_description config_only("Config only options");
    config_only.add_options()
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
    try {
        po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    } catch (po::error& e) {
        log << e.what() << endl;
        throw;
    }

    po::notify(vm);

    if (vm.count("help")) {
        std::ostringstream s;
        s << cmdline_options;
        throw UsageMessage(s.str());
    }

    const string config_path(config.basedir + "/data/settings/freecnc.cfg");
    std::ifstream cfgfile(config_path.c_str());

    po::store(po::parse_config_file(cfgfile, config_file_options), vm);
    po::notify(vm);
}
