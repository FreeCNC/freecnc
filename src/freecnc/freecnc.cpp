//
// Entry point, engine initialization, cleanup
//
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <string>

#include "SDL.h"

#include "freecnc.h"
#include "lib/inifile.h"
#include "game/game_public.h"
#include "renderer/renderer_public.h"
#include "sound/sound_public.h"
#include "vfs/vfs_public.h"

using std::abort;
using std::set_terminate;
using std::runtime_error;

#if defined _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

Logger *logger;

void PrintUsage(); // In args.cpp
bool parse(int argc, char** argv); // In args.cpp

// Below main
void cleanup();
void fcnc_terminate_handler();

int main(int argc, char** argv)
{
    atexit(cleanup);
    set_terminate(fcnc_terminate_handler);

    if ((argc > 1) && ( (strcasecmp(argv[1],"-help")==0) || (strcasecmp(argv[1],"--help")==0)) ) {
        PrintUsage();
        return 1;
    }

    const string& binpath = determineBinaryLocation(argv[0]);
    string lf(binpath);
    lf += "/freecnc.log";

    VFS_PreInit(binpath.c_str());
    // Log level is so that only errors are shown on stdout by default
    logger = new Logger(lf.c_str(),0);
    if (!parse(argc, argv)) {
        return 1;
    }
    const ConfigType& config = getConfig();
    VFS_Init(binpath.c_str());
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
        logger->error("%s\n",e.what());
        #if _WIN32
        MessageBox(0, e.what(), "Fatal error", MB_ICONERROR|MB_OK);
        #endif
    }
    return 0;
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

void cleanup()
{
    delete pc::gfxeng;
    delete pc::sfxeng;
    delete logger;

    VFS_Destroy();
    SDL_Quit();
}
