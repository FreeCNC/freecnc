#ifndef _LIB_DLLIBRARY_H
#define _LIB_DLLIBRARY_H

#include <string>

#ifdef _WIN32
#include <windows.h>
#define LIBHAND HINSTANCE
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#define LIBHAND NSModule
#else
#define LIBHAND void*
#endif

class DLLibrary
{
public:
    DLLibrary(const char *libname);
    ~DLLibrary();
    void *GetFunction(const char *funcname);
    static const char* Extension;

private:
    LIBHAND library;
    std::string name;
};

#endif
