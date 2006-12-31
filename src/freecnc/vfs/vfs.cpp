#include <cstring>
#include <cstdarg>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "../lib/inifile.h"
#include "archive.h"
#include "filesystem/externalvfs.h"
#include "mix/mixvfs.h"
#include "vfile.h"
#include "vfs.h"

using std::runtime_error;

ExternalFiles* externals;
MIXFiles* mixfiles;

/** Sets up externals so that the logger can work
 */
void VFS_PreInit(const char* binpath) 
{
    externals = new ExternalFiles(binpath);
    externals->loadArchive("data/settings/");
}

/** @todo install prefix
 */
void VFS_Init(const char* binpath)
{
    shared_ptr<INIFile> filesini;
    string tempstr;
    unsigned int keynum;

    //logger->debug("Assuming binary is installed in \"%s\"\n", binpath);

    if (strcasecmp(".",binpath)!=0) {
        externals->loadArchive("./");
    }
    tempstr = binpath; tempstr += "/";
    externals->loadArchive(tempstr.c_str());

    #ifndef _WIN32
    externals->loadArchive("/etc/freecnc/");
    #endif

    try {
        filesini = GetConfig("files.ini");
    } catch(runtime_error&) {
        logger->error("Unable to locate files.ini.\n");
        return;
    }
    for (unsigned int pathnum = 1;;++pathnum) {
        INIKey key;
        try {
            key = filesini->readIndexedKeyValue("GENERAL",pathnum,"PATH");
        } catch(int) {
            break;
        }
        string defpath = key->second;
        if (defpath[defpath.length()-1] != '/' &&
            defpath[defpath.length()-1] != '\\') {
            defpath += "/";
        }
        externals->loadArchive(defpath.c_str());
    }

    mixfiles = new MIXFiles();

    for (unsigned int gamenum = 1 ;; ++gamenum) {
        INIKey key;
        try {
            key = filesini->readIndexedKeyValue("GENERAL",gamenum,"GAME");
        } catch(int) {
            break;
        }
        logger->note("Trying to load \"%s\"...\n",key->second.c_str());
        try {
            // First check we have all the required mixfiles.
            for (keynum = 1; ;keynum++) {
                INIKey key2;
                try {
                    key2 = filesini->readIndexedKeyValue(key->second.c_str(),keynum,
                            "REQUIRED");
                } catch(int) {
                    break;
                }
                if( !mixfiles->loadArchive(key2->second.c_str()) ) {
                    logger->warning("Missing required file \"%s\"\n",
                            key2->second.c_str());
                    throw 0;
                }

            }
        } catch(int) {
            mixfiles->unloadArchives();
            continue;
        }
        // Now load as many of the optional mixfiles as we can.
        for (keynum = 1; ;keynum++) {
            INIKey key2;
            try {
                key2 = filesini->readIndexedKeyValue(key->second.c_str(),keynum,
                        "OPTIONAL");
            } catch(int) {
                break;
            }
            mixfiles->loadArchive(key2->second.c_str());
        }
        return;
    }

    logger->error("Unable to find mixes for any of the supported games!\n"
                  "Check your configuration and try again.\n");
    
    #ifdef _WIN32
    MessageBox(0,"Unable to find mixes for any of the supported games!\n"
                 "Check your configuration and try again.","Error",0);
    #endif

    exit(1);
}

void VFS_Destroy()
{
    delete mixfiles;
    delete externals;
}

VFile *VFS_Open(const char *fname, const char* mode)
{
    unsigned int fnum;
    fnum = externals->getFile(fname, mode);
    if( fnum != (unsigned int)-1 ) {
        return new VFile(fnum, externals);
    }
    // Won't attempt to write/create files in real archives
    if (mode[0] != 'r') {
        return NULL;
    }
    if (mixfiles != NULL) {
        unsigned int fnum = mixfiles->getFile(fname);
        if (fnum != (unsigned int)-1) {
            return new VFile(fnum, mixfiles);
        }
    }
    return NULL;
}

void VFS_Close(VFile *file)
{
    delete file;
}

const char* VFS_getFirstExisting(const vector<const char*>& files)
{
    VFile* tmp;
    for (unsigned int i=0;i<files.size();++i) {
        tmp = VFS_Open(files[i],"r");
        if (tmp != NULL) {
            VFS_Close(tmp);
            return files[i];
        }
    }
    return NULL;
}

const char* VFS_getFirstExisting(unsigned int count, ...)
{
    VFile* tmp;
    va_list ap;
    char* name;
    va_start(ap,count);
    while (count--) {
        name = (char*)va_arg(ap,char*);
        tmp = VFS_Open(name);
        if (tmp != NULL) {
            VFS_Close(tmp);
            va_end(ap);
            return name;
        }
    }
    va_end(ap);
    return NULL;
}

void VFS_LoadGame(GameType gt)
{
    switch (gt) {
    case GAME_TD:
        externals->loadArchive("data/settings/td/");
        break;
    case GAME_RA:
        externals->loadArchive("data/settings/ra/");
        break;
    default:
        logger->error("Unknown gametype %i specified\n", gt);
        break;
    }
}

