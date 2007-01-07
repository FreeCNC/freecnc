#include <cstdio>
#include <cstdlib>

#include "../freecnc.h"
#include "../renderer/renderer_public.h"

Logger::Logger()
{
    rendergamemsg = false;
}

void Logger::gameMsg(const char *txt, ...)
{
    char msgstring[64];
    va_list ap;

    va_start(ap, txt);
#ifdef _MSC_VER
    _vsnprintf(msgstring, 64, txt, ap);
#else
    vsnprintf(msgstring, 64, txt, ap);
#endif
    va_end(ap);
    game.log << "Game: " << msgstring << endl;

    if(rendergamemsg && (pc::msg != NULL)) {
        pc::msg->postMessage(msgstring);
    }
}
