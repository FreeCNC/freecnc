#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

#include "SDL.h"

#include "../freecnc.h"
#include "inifile.h"

using std::runtime_error;

namespace {
    // Nameless namespace to hide this.
    ConfigType config;
}

/** Print the help message */
void PrintUsage()
{
    printf("FreeCNC - %s\n\n", VERSION);
    printf("Usage: freecnc [OPTIONS]\n");
    printf("  -basedir dirname   - Specify the location of the data directory\n");
    printf("  -map mapname       - Name of mission to load\n");
    printf("  -w width           - Width of screen\n");
    printf("  -h height          - Height of screen\n");
    printf("  -bpp bpp           - Video Depth\n");
    printf("  -fullscreen        - Use fullscreen mode\n");
    printf("  -window            - Use windowed mode\n");
    printf("  -nosound           - Play without sound\n");
    printf("  -playvqa vqaname   - Plays a VQA\n");
    printf("  -grab              - Grabs mouse input (locks mouse inside freecnc window)\n\n");
    printf("The following options are for features that are in development:\n");
    printf("  -skirmish N        - Starts up in skirmish mode with N players\n");
    printf("  -multi X Y         - Starts up in multiplayer mode as player X of Y\n");
    printf("  -nick nickname     - Sets your nick for multiplayer\n");
    printf("  -colour colourname - Sets your side colour for multiplayer\n");
    printf("allowed colours: red, orange, yellow, green, blue and turquoise\n");
    printf("  -side <GDI or NOD> - sets your side for multiplayer\n");
    printf("  -server address    - Address of the server for multiplayer.\n");
    printf("  -port number       - Port to which a connection should be made.\n\n");
}

const ConfigType& getConfig()
{
    return config;
}

// Parses command line arguments
// argc: Argument count
// argv: Array of char arrays of arguments
// returns: true on success, false if invalid parameters have been provided
bool parse(int argc, char **argv)
{
    int i;
    char *tmp;
    bool fullscreen = false;
    shared_ptr<INIFile> freecnc_ini;
    shared_ptr<INIFile> internal_ini;

    try {
        freecnc_ini = GetConfig("freecnc.ini");
        internal_ini = GetConfig("internal-global.ini");
    } catch(runtime_error& e) {
        logger->error("%s\n",e.what());
        return false;
    }

    /* Setup defaults */
    /* Some of the "defaults" are in freecnc.ini */
    config.videoflags = SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_HWPALETTE;

    tmp = freecnc_ini->readString("Options", "Map");
    if (tmp == 0) {
        logger->error("Option \"Map\" missing in inifile\n");
        return false;
    }
    config.mapname = tmp;
    delete[] tmp;
    config.width = freecnc_ini->readInt("Video", "Width", 640);
    config.height = freecnc_ini->readInt("Video", "Height", 480);
    config.bpp = freecnc_ini->readInt("Video", "Bpp", 16);
    fullscreen = freecnc_ini->readInt("Video","fullscreen",0);
    config.intro = freecnc_ini->readInt("Options", "PlayIntro", 1);
    config.gamenum = (gametypes)freecnc_ini->readInt("Options", "Game", GAME_TD);
    config.nosound = (freecnc_ini->readInt("Options", "Nosound",0) != 0);
    config.playvqa = false;
    config.gamemode = 0;
    config.serverport = 1995;
    config.finaldelay = freecnc_ini->readInt("Options","FinalDelay",100);
    config.scrollstep = freecnc_ini->readInt("Options","ScrollStep",1);
    config.scrolltime = freecnc_ini->readInt("Options","ScrollTime",5);
    config.maxscroll  = freecnc_ini->readInt("Options","MaxScroll",24);
    config.buildable_radius = internal_ini->readInt("Rules","buildable_radius",2);
    // if this is lower than 2, it makes placing the refinery difficult
    config.buildable_radius = max((unsigned char)2,config.buildable_radius);
    config.buildable_ratio  = (internal_ini->readInt("Rules","buildable_ratio",70))/100.0;
    config.bindablekeys[KEY_SIDEBAR] = SDLK_TAB;
    config.bindablemods[KEY_SIDEBAR] = 0;
    config.bindablekeys[KEY_STOP] = SDLK_s;
    config.bindablemods[KEY_STOP] = 0;
    config.bindablekeys[KEY_ALLY] = SDLK_a;
    config.bindablemods[KEY_ALLY] = 0;
    config.dispatch_mode = 0;
    config.debug = freecnc_ini->readInt("Options", "Debug", 0) != 0;

    config.serveraddr = "127.0.0.1";
    config.grabmode = SDL_GRAB_OFF;

    for (i = 1; i < argc; i++) {
        if ( strcmp(argv[i], "-basedir") == 0 ) {
            ++i;
            continue;
        }
        if ( strcmp(argv[i], "-nosound") == 0) {
            config.nosound = true;
            continue;
        }
        if ( strcmp(argv[i], "-fullscreen") == 0 ) {
            fullscreen = 1;
            continue;
        }
        if ( strcmp(argv[i], "-window") == 0 ) {
            fullscreen = 0;
            continue;
        }

        if ( strcmp(argv[i], "-ra") == 0 ) {
            config.gamenum = GAME_RA;
            continue;
        }
        if (strcmp(argv[i], "-playvqa") == 0) {
            if (argv[i+1]) {
                config.playvqa = true;
                config.vqamovie = argv[i+1];
                i++;
            }
            continue;
        }
        if (strcmp(argv[i], "-skirmish") == 0) {
            if (argv[i+1]) {
                config.gamemode = 1;
                config.totalplayers = abs(atoi(argv[i+1]));
                if (config.totalplayers <= 1) {
                    config.totalplayers = 2;
                }
                config.playernum = 1;
                ++i;
                if (config.totalplayers > MAXPLAYERS) {
                    logger->warning("Sorry, the maximum number of players is %i\n",MAXPLAYERS);
                    config.totalplayers = MAXPLAYERS;
                }
            }
            continue;
        }
        if (strcmp(argv[i], "-multi") == 0) {
            if (argv[i+1]&&(argv[i+2])) {
                config.gamemode = 2;
                config.totalplayers = abs(atoi(argv[i+2]));
                if (config.totalplayers <= 1) {
                    config.totalplayers = 2;
                }
                if (config.totalplayers > MAXPLAYERS) {
                    logger->warning("Sorry, the maximum number of players is %i\n",MAXPLAYERS);
                    config.totalplayers = MAXPLAYERS;
                }
                config.playernum = abs(atoi(argv[i+1]));
                if (config.playernum < 1) {
                    config.playernum = 1;
                } else if (config.playernum > config.totalplayers) {
                    config.playernum = config.totalplayers;
                }
                i += 2;
            }
            continue;
        }
        if (strcmp(argv[i], "-nick") == 0) {
            if (argv[i+1]) {
                config.nick = argv[i+1];
                ++i;
            }
            continue;
        }
        if (strcmp(argv[i], "-colour") == 0) {
            if (argv[i+1]) {
                config.side_colour = argv[i+1];
                ++i;
            }
            continue;
        }
        if (strcmp(argv[i], "-side") == 0) {
            if (argv[i+1]) {
                config.mside = argv[i+1];
                ++i;
            }
            continue;
        }
        if (strcmp(argv[i], "-server") == 0) {
            if (argv[i+1]) {
                config.serveraddr = argv[i+1];
                ++i;
            }
            continue;
        }
        if (strcmp(argv[i], "-port") == 0) {
            if (argv[i+1]) {
                config.serverport = abs(atoi(argv[i+1]));
                ++i;
            }
            continue;
        }
        if (strcmp(argv[i], "-grab") == 0) {
            config.grabmode = SDL_GRAB_ON;
            continue;
        }
        if ( strcmp(argv[i], "-map") == 0 ) {
            if (argv[i+1]) {
                config.mapname = argv[i+1];
                std::transform(config.mapname.begin(), config.mapname.end(), config.mapname.begin(), toupper);
                ++i;
            }
            continue;
        }

        if ( strcmp(argv[i], "-w") == 0 ) {
            if (argv[i+1]) {
                config.width = atoi(argv[i+1]);
                i++;
            }
            continue;
        }

        if ( strcmp(argv[i], "-h") == 0 ) {
            if (argv[i+1]) {
                config.height = atoi(argv[i+1]);
                i++;
            }
            continue;
        }

        if ( strcmp(argv[i], "-bpp") == 0 ) {
            if (argv[i+1]) {
                config.bpp = atoi(argv[i+1]);
                i++;
            }
            continue;
        }

        /*
        if ( strcmp(argv[i], "-record") == 0 ) {
            if (argv[i+1]) {
                config.dispatch_mode = 1;
                config.disp_logname = argv[i+1];
                i++;
            } else {
                return false;
            }
            continue;
        }
        if ( strcmp(argv[i], "-play") == 0) {
            if (argv[i+1]) {
                config.dispatch_mode = 2;
                config.disp_logname = argv[i+1];
                i++;
            } else {
                return false;
            }
            continue;
        }
        */

        if ( strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0)  {
            /* -help prints the help message and returns -1 to stop execution*/
            PrintUsage();
            return false;
        }
        logger->error("Unknown argument: %s, exiting\n",argv[i]);
        return false;
    }
    config.videoflags |= (fullscreen?SDL_FULLSCREEN:0);

    return true;
}

