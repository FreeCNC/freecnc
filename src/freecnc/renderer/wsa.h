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
    SDL_Surface *decodeFrame(unsigned short framenum);
    unsigned char *wsadata;
    unsigned char *framedata;
    SDL_Color palette[256];
    unsigned char loop; /* Whether WSA loops or not */
    char *sndfile;
    struct WSAHeader {
        unsigned short NumFrames;
        unsigned short xpos;
        unsigned short ypos;
        unsigned short width;
        unsigned short height;
        unsigned int delta;
        unsigned int *offsets;
    };
    WSAHeader header;
};

#endif
