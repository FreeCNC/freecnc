#ifndef _FREECNC_H
#define _FREECNC_H
//
// Defines the common types and the global variables
//

#include <cstring>

#include "SDL_video.h"  // Needs to go
#include "SDL_keysym.h" // Needs to go

#include "basictypes.h"
#include "gameengine.h"
#include "version.h"
#include "lib/logger.h" // Remove ASAP

extern GameEngine game;
extern Logger* logger; // Remove ASAP

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
const unsigned char MAXPLAYERS = 6;

const unsigned short FULLHEALTH = 256;

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
    unsigned int animdelay;
    unsigned char loopend, loopend2, animspeed, animtype, sectype;
    unsigned char dmgoff, dmgoff2;
    unsigned short makenum;
};

/// @TODO: This shouldn't be here
struct powerinfo_t {
    unsigned short power;
    unsigned short drain;
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
vector<char*> splitList(char* line, char delim);

/// @TODO Stringify this funciton
char* stripNumbers(const char* src);

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

#endif
