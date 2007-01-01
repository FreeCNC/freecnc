#include "../freecnc.h"
#include "dllibrary.h"

// ----------------------------------------------------------------------------
// Windows
// ----------------------------------------------------------------------------

#ifdef _WIN32

const char *DLLibrary::Extension = "dll";

DLLibrary::DLLibrary(const char *libname) : name(libname)
{
    library = LoadLibrary(libname);
    if (!library) {
        game.log << "Unable to load library \"" << libname << "\""  << endl;
        throw 0;
    }
}

DLLibrary::~DLLibrary()
{
    FreeLibrary(library);
}

void *DLLibrary::GetFunction(const char *funcname)
{
    void *retval = (void *)GetProcAddress(library, funcname);
    if (retval == NULL) {
        game.log << "Unable to extract function \"" << funcname << "\" from library \"" << name << "\"" << endl;
    }
    return retval;
}

// ----------------------------------------------------------------------------
// Mac OS X
// ----------------------------------------------------------------------------

#elif defined(__APPLE__)

const char *DLLibrary::Extension = "so";

DLLibrary::DLLibrary(const char *libname) : name(libname)
{
    NSObjectFileImage image;

    if (NSCreateObjectFileImageFromFile(libname, &image) == NSObjectFileImageSuccess) {
        library = NSLinkModule(image, libname,
                               NSLINKMODULE_OPTION_RETURN_ON_ERROR|
                               NSLINKMODULE_OPTION_PRIVATE);
        if (!library) {
            NSLinkEditErrors ler;
            int lerno;
            const char* errstr;
            const char* file;
            NSLinkEditError(&ler,&lerno,&file,&errstr);
            game.log << "Unable to load library \"" << libname << "\": " << errstr << endl;
            throw 0;
        }
        NSDestroyObjectFileImage(image);
    } else {
        game.log << "Unable to load library \"" << libname << "\""  << endl;
        throw 0;
    }
}

DLLibrary::~DLLibrary()
{
    NSUnLinkModule(library, 0);
}

void *DLLibrary::GetFunction(const char* funcname)
{
    char* fsname = new char[strlen(funcname)+2];
    sprintf(fsname, "_%s", funcname);
    NSSymbol sym = NSLookupSymbolInModule(library, fsname);
    delete[] fsname;
    if (sym) {
        return NSAddressOfSymbol(sym);
    }
    game.log << "Unable to extract function \"" << funcname << "\" from library \"" << name << "\"" << endl;
    return NULL;
}

// ----------------------------------------------------------------------------
// POSIX
// ----------------------------------------------------------------------------

#else
#include <cstdio>
#include <dlfcn.h>
#ifdef RTLD_NOW
#define LIBOPTION RTLD_NOW
#elif defined(RTLD_LAZY)
#define LIBOPTION RTLD_LAZY
#elif defined(DL_LAZY)
#define LIBOPTION DL_LAZY
#else
#define LIBOPTION 0
#endif

const char *DLLibrary::Extension = "so";

DLLibrary::DLLibrary(const char *libname) : name(libname)
{
    library = dlopen(libname, LIBOPTION);
    if( !library ) {
        game.log << "Unable to load library \"" << libname << "\": " << dlerror() << endl;
        throw 0;
    }
}

DLLibrary::~DLLibrary()
{
    dlclose(library);
}

void *DLLibrary::GetFunction(const char *funcname)
{
    void *retval = (void*)dlsym(library, funcname);
    //FIXME: Some dynamic loaders require a prefix on the shared names.
    //       the only one I've encountered is "_" which is required on openBSD
    //       and possible some other platform. This should be detected at
    //       compiletime but libs are loaded so seldom this will have to do
    //       for now.
    if (retval == NULL) {
        char* fsname;
        fsname = new char[strlen(funcname)+2];
        sprintf(fsname, "_%s", funcname);
        retval = (void*)dlsym(library, fsname);
        delete[] fsname;
    }
    if (retval == NULL) {
        game.log << "Unable to extract function \"" << funcname << "\" from library \"" << name << "\": " << dlerror() << endl;
    }
    return retval;
}

#endif

