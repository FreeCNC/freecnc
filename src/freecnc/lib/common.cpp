#include <iostream>
#include <string>
#include <cstdio>
#include <cctype>

#include "inifile.h"

using std::cout;

int mapscaleq = -1;
namespace {
    string binloc;
}

namespace p {
    ActionEventQueue* aequeue = 0;
    CnCMap* ccmap = 0;
    UnitAndStructurePool* uspool = 0;
    PlayerPool* ppool = 0;
    WeaponsPool* weappool = 0;
    Dispatcher::Dispatcher* dispatcher = 0;
    map<string, shared_ptr<INIFile> > settings;
}

// We pass by value because we could copy anyway
shared_ptr<INIFile> GetConfig(string name) {
    transform(name.begin(), name.end(), name.begin(), tolower);
    map<string, shared_ptr<INIFile> >::const_iterator it = p::settings.find(name);
    if (p::settings.end() == it) {
        shared_ptr<INIFile> ret(new INIFile(name.c_str()));
        p::settings[name] = ret;
        return ret;
    }
    return it->second;
}

// Client only
namespace pc {
    SoundEngine* sfxeng = 0;
    GraphicsEngine* gfxeng = 0;
    MessagePool* msg = 0;
    std::vector<SHPImage *>* imagepool = 0;
    ImageCache* imgcache = 0;
    Sidebar* sidebar = 0;
    Cursor* cursor = 0;
    Input* input = 0;
}

// Server only
namespace ps {
}

// Server only
std::vector<char*> splitList(char* line, char delim)
{
    std::vector<char*> retval;
    char* tmp;
    unsigned int i,i2;
    tmp = NULL;
    if (line != NULL) {
        tmp = new char[16];
        memset(tmp,0,16);
        for (i=0,i2=0;line[i]!=0x0;++i) {
            if ( (i2>=16) || (tmp != NULL && (line[i] == delim)) ) {
                retval.push_back(tmp);
                tmp = new char[16];
                memset(tmp,0,16);
                i2 = 0;
            } else {
                tmp[i2] = line[i];
                ++i2;
            }
        }
        retval.push_back(tmp);
    }
    return retval;
}

/// change "foo123" to "foo"
char* stripNumbers(const char* src)
{
    char* dest;
    unsigned short i;
    for (i=0;i<strlen(src);++i) {
        if (src[i] <= '9') {
            break;
        }
    }
    dest = new char[i+1];
    strncpy(dest,src,i);
    dest[i] = 0;
    return dest;
}

char normalise_delim(char c) {
    if ('\\' == c) {
        return '/';
    }
    return c;
}

/// @TODO Something's not right, but this works better.
const string& determineBinaryLocation(const string& launchcmd) {
    string path(launchcmd);

    transform(path.begin(), path.end(), path.begin(), normalise_delim);
    string::size_type delim = path.find_last_of('/');

    if (string::npos == delim) {
        return binloc = ".";
    }
    return binloc = path.substr(0, delim);
}

const string& getBinaryLocation() {
    return binloc;
}
