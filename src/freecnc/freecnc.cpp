//
// Entry point, engine initialization, cleanup
//
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "SDL.h"

#include "freecnc.h"
#include "lib/inifile.h"
#include "game/game_public.h"
#include "renderer/renderer_public.h"
#include "sound/sound_public.h"
#include "vfs/vfs_public.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

using std::cout;
using std::cerr;
using std::abort;
using std::set_terminate;
using std::runtime_error;

#if defined _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sstream>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

Logger *logger;
GameEngine game;

void fcnc_error(const char* message);
void fcnc_info(const char* message);
void cleanup();
void fcnc_terminate_handler();
bool parse_options(int argc, char** argv);
// TEMP
bool parse(int argc, char** argv);

int main(int argc, char** argv)
{
    atexit(cleanup);
    set_terminate(fcnc_terminate_handler);

    if (!parse_options(argc, argv)) {
        return 1;
    }
    string lf(game.config.basedir);
    lf += "/freecnc.log";

    VFS_PreInit(game.config.basedir.c_str());
    // Log level is so that only errors are shown on stdout by default
    logger = new Logger(lf.c_str(),0);
    if (!parse(argc, argv)) {
        return 1;
    }
    const ConfigType& config = getConfig();
    VFS_Init(game.config.basedir.c_str());
    VFS_LoadGame(config.gamenum);
    logger->note("Please wait, FreeCNC %s is starting\n", VERSION);


    try {
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
            logger->error("Couldn't initialize SDL: %s\n", SDL_GetError());
            exit(1);
        }

        if (config.debug) {
            // Don't hide if we're debugging as the lag when running inside
            // valgrind is really irritating.
            logger->debug("Debug mode is enabled\n");
        } else {
            // Hide the cursor since we have our own.
            SDL_ShowCursor(0);
        }

        // Initialise Video
        try {
            logger->note("Initialising the graphics engine...");
            pc::gfxeng = new GraphicsEngine();
            logger->note("done\n");
        } catch (GraphicsEngine::VideoError&) {
            logger->note("failed.  exiting\n");
            throw runtime_error("Unable to initialise the graphics engine");
        }

        // Initialise Sound
        logger->note("Initialising the sound engine...");
        pc::sfxeng = new SoundEngine(config.nosound);
        logger->note("done\n");


        // "Standalone" VQA Player
        if (config.playvqa) {
            logger->note("Now playing %s\n", config.vqamovie.c_str());
            unsigned char ret = 0;
            try {
                VQAMovie mov(config.vqamovie.c_str());
                mov.play();
            } catch (runtime_error&) {
                ret = 1;
            }
            exit(ret);
        }

        // Play the intro if requested
        if (config.intro) {
            try {
                VQAMovie mov(config.gamenum != GAME_RA ? "logo" : "prolog");
                mov.play();
            } catch (runtime_error&) {
            }

            try {
                WSA choose("choose.wsa");
                choose.animate();
            } catch (WSA::WSAError&) {
            }
        }

        // Init the rand functions
        srand(static_cast<unsigned int>(time(0)));

        // Initialise game engine
        try {
            logger->note("Initialising game engine:\n");
            Game gsession;
            logger->note("Starting game\n");
            gsession.play();
            logger->note("Shutting down\n");
        } catch (Game::GameError&) {
        }
    } catch (runtime_error& e) {
        fcnc_error(e.what());
    }
    return 0;
}

void fcnc_error(const char* message)
{
    #if _WIN32
    MessageBox(0, message, "Fatal error", MB_ICONERROR|MB_OK);
    #else
    cerr << "Fatal Error: " << message << "\n";
    #endif
}

void fcnc_info(const char* message)
{
    #if _WIN32
    MessageBox(0, message, "FreeCNC", MB_ICONINFORMATION|MB_OK);
    #else
    cout << message << "\n";
    #endif
}

void cleanup()
{
    delete pc::gfxeng;
    delete pc::sfxeng;
    delete logger;

    VFS_Destroy();
    SDL_Quit();
}
// Wraps around a more verbose terminate handler and cleans up better
void fcnc_terminate_handler()
{
    cleanup();
    #if __GNUC__ == 3 && __GNUC_MINOR__ >= 1
    // GCC 3.1+ feature, and is turned on by default for 3.4.
    using __gnu_cxx::__verbose_terminate_handler;
    __verbose_terminate_handler();
    #else
    abort();
    #endif
}

bool parse_options(int argc, char** argv)
{
    GameConfig& config(game.config);

    po::options_description generic("Generic options");
    generic.add_options()
        ("help,h", "show this message")
        ("version,v", "print version")
        ("basedir", po::value<string>(&config.basedir)->default_value("."),
            "use this location to find the data files")
    ;

    po::options_description game("Game options");
    game.add_options()
        ("map", po::value<string>(&config.map)->default_value("SCG01EA"),
            "start on this mission")
        ("fullscreen", "start fullscreen")
        ("width,w", po::value<int>(&config.width)->default_value(640),
            "use this value for screen width")
        ("height,h", po::value<int>(&config.height)->default_value(480),
            "use this value for screen height")
    ;

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
            "maximum speed for scrolling")
    ;

    po::options_description debug("Debug options");
    debug.add_options()
        ("nosound", "disable sound")
        ("debug", "turn on various internal debugging features")
    ;

    po::options_description cmdline_options, config_file_options;

    cmdline_options.add(generic).add(game).add(debug);
    config_file_options.add(game).add(config_only).add(debug);

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
    } catch (po::error& e) {
        fcnc_error(e.what());
        return false;
    }

    po::notify(vm);

    if (vm.count("help")) {
        std::ostringstream s;
        s << cmdline_options;
        fcnc_info(s.str().c_str());
        return false;
    }

    const string config_path(config.basedir + "/freecnc.cfg");
    std::ifstream cfgfile(config_path.c_str());

    po::store(po::parse_config_file(cfgfile, config_file_options), vm);
    po::notify(vm);

    config.fullscreen = vm.count("fullscreen");

    return true;
}
