#include <cstdio>
#include <cstdlib>

#include "../freecnc.h"
#include "../renderer/renderer_public.h"
#include "../vfs/vfs_public.h"

Logger::Logger(const char *logname, int threshold)
{
    logfile = VFS_Open(logname, "w");
    
    #ifndef _WIN32
    if( logfile == NULL ) {
        printf("Unable to open logfile \"%s\" for writing, using stdout instead\n", logname);
    }
    #endif

    indentsteps = 0;
    indentString[0] = '\0';
    rendergamemsg = false;
    this->threshold = threshold;
}

Logger::~Logger()
{
    VFS_Close(logfile);
}

void Logger::indent()
{
    if (indentsteps > 60) {
        indentsteps = 62;
    } else {
        indentString[indentsteps] = ' ';
        indentString[indentsteps+1] = ' ';
        indentsteps+=2;
    }
    indentString[indentsteps] = '\0';
}

void Logger::unindent()
{
    if (indentsteps < 3) {
        indentsteps = 0;
    } else {
        indentsteps -= 2;
    }
    indentString[indentsteps] = '\0';
}

void Logger::error(const char *txt, ...)
{
    va_list ap, aq;
    va_start(ap, txt);
    va_start(aq, txt);
    if (logfile != NULL) {
        logfile->vfs_printf("%sERROR: ", indentString);
        logfile->vfs_vprintf(txt, aq);
        logfile->flush();
    }
    
    #ifndef _WIN32
    printf("%sERROR: ", indentString);
    vprintf(txt, ap);
    fflush(stdout);
    #endif

    va_end(aq);
    va_end(ap);
}

void Logger::debug(const char *txt, ...)
{
    va_list ap, aq;
    va_start(ap, txt);
    va_start(aq, txt);
    if (logfile != NULL) {
        logfile->vfs_printf("%sDEBUG: ", indentString);
        logfile->vfs_vprintf(txt, aq);
        logfile->flush();
    }
    
    #ifndef _WIN32
    printf("%sDEBUG: ", indentString);
    vprintf(txt, ap);
    fflush(stdout);
    #endif

    va_end(aq);
    va_end(ap);
}

void Logger::warning(const char *txt, ...)
{
    va_list ap, aq;
    va_start(ap, txt);
    va_start(aq, txt);
    if (logfile != NULL) {
        logfile->vfs_printf("%sWARNING: ", indentString);
        logfile->vfs_vprintf(txt, aq);
        logfile->flush();
    }
    
    #ifndef _WIN32
    if (threshold > 0) {
        printf("%sWARNING: ", indentString);
        vprintf(txt,ap);
        fflush(stdout);
    }
    #endif

    va_end(aq);
    va_end(ap);
}

void Logger::note(const char *txt, ...)
{
    va_list ap, aq;
    va_start(ap, txt);
    va_start(aq, txt);
    if (logfile != NULL) {
        logfile->vfs_printf("%s", indentString);
        logfile->vfs_vprintf(txt, aq);
        logfile->flush();
    }

    #ifndef _WIN32
    if (threshold > 1) {
        printf("%s", indentString);
        vprintf(txt,ap);
        fflush(stdout);
    }
    #endif

    va_end(aq);
    va_end(ap);
}

void Logger::gameMsg(const char *txt, ...)
{
    va_list ap;
    va_start(ap, txt);
    if (logfile != NULL) {
        logfile->vfs_printf("%sGame: ", indentString);
        logfile->vfs_vprintf(txt, ap);
        logfile->vfs_printf("\n");
        logfile->flush();
    }
    va_end(ap);

    if(rendergamemsg && (pc::msg != NULL)) {
        char msgstring[64];
        va_start(ap, txt);
        #ifdef _MSC_VER
        _vsnprintf(msgstring, 64, txt, ap);
        #else
        vsnprintf(msgstring, 64, txt, ap);
        #endif
        va_end(ap);
        pc::msg->postMessage(msgstring);
    } else {
        #ifndef _WIN32
        va_start(ap, txt);
        printf("%s",indentString);
        vprintf(txt,ap);
        va_end(ap);
        printf("\n");
        fflush(stdout);
        #endif
    }
}
