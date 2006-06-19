#ifndef _RENDERER_WSAIMAGE_H
#define _RENDERER_WSAIMAGE_H

#include "SDL.h"

#include "../freecnc.h"

class WSA{
public:
    WSA(const char* fname);
    ~WSA();
    void animate();
    class WSAError {};
private:
    SDL_Surface *decodeFrame(Uint16 framenum);
    Uint8 *wsadata;
    Uint8 *framedata;
    SDL_Color palette[256];
    Uint8 loop; /* Whether WSA loops or not */
    char *sndfile;
    struct WSAHeader {
        Uint16 NumFrames;
        Uint16 xpos;
        Uint16 ypos;
        Uint16 width;
        Uint16 height;
        Uint32 delta;
        Uint32 *offsets;
    };
    WSAHeader header;
};

#endif
