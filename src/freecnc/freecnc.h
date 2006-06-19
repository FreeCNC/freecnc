#ifndef _FREECNC_H
#define _FREECNC_H
//
// Defines the primitive types, common types and the global variables
//
// TODO: Get rid of everything below OLD, implement GameEngine

#include <map>
#include <string>
#include <vector>

#include <boost/array.hpp>
#include <boost/smart_ptr.hpp>

#include "SDL_types.h"  // Needs to go
#include "SDL_video.h"  // Needs to go
#include "SDL_keysym.h" // Needs to go

#include "version.h"
#include "lib/endian.h"
#include "lib/logger.h"

// ----------------------------------------------------------------------------
// New
// ----------------------------------------------------------------------------

// TODO: Migrate FreeCNC to use these instead of any libs (currently SDL)
typedef unsigned char      byte;
typedef signed short       int16;
typedef signed long        int32;
typedef signed long long   int64;
typedef unsigned short     uint16;
typedef unsigned long      uint32;
typedef unsigned long long uint64;

using std::map;
using std::string;
using std::vector;

using std::max;
using std::min;

using boost::array;
using boost::scoped_ptr;
using boost::shared_ptr;

/*
class InputEngine;
class NetworkClient;
class NetworkServer;
class Renderer;
class SoundEngine;

class GameMode 
{
public:
    virtual ~GameMode();
    virtual void Update() = 0;
    virtual void Render() = 0;    
};

class GameEngine
{
public:
    GameEngine() : running(false), inited(false) {}
    ~GameEngine() { Shutdown(); }

    // Initializes or reconfigures all subsytems.
    // It uses the settings in config to do so.
    void Init();

    // Shuts down the engine explicitly.
    void Shutdown();

    // The heart of the engine: Basically just does a bit of bookkeeping and
    // calls Update and Render on the current GameMode.
    void MainLoop();

    // Pushes a new mode on top of the current; When it is popped, the previous
    // mode is resumed.
    void PushMode(shared_ptr<GameMode> mode);

    // Pops the current mode
    void PopMode();

    // Returns the current mode
    shared_ptr<GameMode> CurrentMode();

    // The input subsystem, null if in dedicated-server mode
    scoped_ptr<InputEngine> input;

    // The networking subsystem
    scoped_ptr<NetworkClient> network;

    // The server networking subsytem, null if not hosting a server
    scoped_ptr<NetworkServer> serverNetwork;
    
    // The rendering subsystem, null if in dedicated-server mode
    scoped_ptr<Renderer> renderer;

    // The sound subsystem, null if in dedicated-server mode
    scoped_ptr<SoundEngine> sound;

    // Engine configuration. Used by Init.
    Config config;

    // The logger - used like a stream: game.log << "test" << endl;
    Logger log;

    // Virtual Filesystem, used for nearly all file access; Provides archive
    // abstraction.
    VFS vfs;

private:
    // Whether the engine has been initialized
    bool inited;

    // Whether the engine is currently running (in MainLoop)
    bool running;

    // The configuration used for the latest init. Used to determine which
    // settings have changed when re-initing, and to support recovery.
    Config curconfig;

    // The current GameModes.
    std::stack<shared_ptr<GameMode> > modes;
};

extern GameEngine game;
*/

// Ideally, after the big refactoring, everything below here will be gone

// ----------------------------------------------------------------------------
// Old
// ----------------------------------------------------------------------------

extern Logger *logger;

// Forward declarations
class ActionEventQueue;
class CnCMap;
class Cursor;
class GraphicsEngine;
class ImageCache;
class INIFile;
class Input;
class MessagePool;
class PlayerPool;
class SHPImage;
class Sidebar;
class SoundEngine;
class UnitAndStructurePool;
class WeaponsPool;

namespace Dispatcher {
    class Dispatcher;
}

// Used by both client and server
namespace p
{
    extern ActionEventQueue *aequeue;
    extern CnCMap *ccmap;
    extern UnitAndStructurePool *uspool;
    extern PlayerPool *ppool;
    extern WeaponsPool *weappool;
    extern Dispatcher::Dispatcher *dispatcher;
    extern std::map<std::string, shared_ptr<INIFile> > settings;
}

// Client only
namespace pc
{
    extern SoundEngine *sfxeng;
    extern GraphicsEngine *gfxeng;
    extern MessagePool *msg;
    extern std::vector<SHPImage *> *imagepool;
    extern ImageCache *imgcache;
    extern Sidebar *sidebar;
    extern Cursor *cursor;
    extern Input *input;
}

// Server only
namespace ps 
{
    // To come
}

shared_ptr<INIFile> GetConfig(std::string name);

#ifdef _MSC_VER
#define strcasecmp(str1, str2) _stricmp((str1), (str2))
#define strncasecmp(str1, str2, count) _strnicmp((str1), (str2), (count))
#endif

// Bounded by colours.  This will change later
const Uint8 MAXPLAYERS = 6;

enum gametypes {GAME_TD = 1, GAME_RA = 2};

const Uint16 FULLHEALTH = 256;

extern int mapscaleq;

#ifdef M_PI
#undef M_PI
#endif
#define M_PI   3.14159265358979323846

#ifdef M_PI_2
#undef M_PI_2
#endif
#define M_PI_2 1.57079632679489661923

/// @TODO: This shouldn't be here
struct animinfo_t {
    Uint32 animdelay;
    Uint8 loopend, loopend2, animspeed, animtype, sectype;
    Uint8 dmgoff, dmgoff2;
    Uint16 makenum;
};

/// @TODO: This shouldn't be here
struct powerinfo_t {
    Uint16 power;
    Uint16 drain;
    bool powered;
};

/// @TODO: This shouldn't be here
enum PSIDE {
    PS_UNDEFINED = 0, PS_GOOD = 0x1, PS_BAD = 0x2,
    PS_NEUTRAL = 0x4, PS_SPECIAL = 0x8, PS_MULTI = 0x10
};

/// @TODO: This shouldn't be here
enum armour_t {
    AC_none = 0, AC_wood = 1, AC_light = 2, AC_heavy = 3, AC_concrete = 4
};

/// @TODO: This shouldn't be here
enum LOADSTATE {
    PASSENGER_NONE = 0, PASSENGER_LOAD = 1, PASSENGER_UNLOAD = 2
};

/// Same as strdup but uses C++ style allocation
/// @TODO: Eliminate this function
inline char* cppstrdup(const char* s) {
    char* r = new char[strlen(s)+1];
    return strcpy(r,s);
}

inline bool isRelativePath(const char *p) {
#ifdef _WIN32
    return ((strlen(p) == 0) || p[1] != ':') && p[0] != '\\' && p[0] != '/';
#else
    return p[0] != '/';
#endif
}

/// @TODO Stringify this funciton
std::vector<char*> splitList(char* line, char delim);

/// @TODO Stringify this funciton
char* stripNumbers(const char* src);

const std::string& determineBinaryLocation(const std::string& launchcmd);

const std::string& getBinaryLocation();

/** What each state means:
 * INVALID: Build Queue in an inconsistent state, expect "undefined behaviour".
 * EMPTY: Nothing to construct.
 * RUNNING: Construction is running.  Should proceed to either READY or PAUSED.
 * PAUSED: Construction paused.  Requires user intervention to return to RUNNING.
 * READY: Construction finished, waiting for destination to be confirmed.
 * CANCELLED: Status shouldn't be set to this, but is used to inform the ui
 * when production is cancelled.
 */
enum ConStatus {BQ_INVALID, BQ_EMPTY, BQ_RUNNING, BQ_PAUSED, BQ_READY, BQ_CANCELLED};

class InfantryGroup;
typedef shared_ptr<InfantryGroup> InfantryGroupPtr;


enum KEY_TYPE {
    KEY_SIDEBAR = 0, KEY_STOP, KEY_ALLY
};

struct ConfigType
{
    Uint32 videoflags;
    Uint16 width, height, bpp, serverport;
    Uint8 intro, gamemode, totalplayers, playernum,
        scrollstep, scrolltime, maxscroll, finaldelay, dispatch_mode;
    bool nosound, playvqa, allowsandbagging, debug;
    gametypes gamenum;
    SDL_GrabMode grabmode;
    static const Uint8 NUMBINDABLE = 3;
    SDLKey bindablekeys[NUMBINDABLE];
    Uint8 bindablemods[NUMBINDABLE];
    Uint8 buildable_radius;
    std::string mapname, vqamovie, nick, side_colour, mside, serveraddr; // ,disp_logname;
    double buildable_ratio;
};

const ConfigType& getConfig();

#endif
