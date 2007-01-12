#ifndef _RENDERER_WSAIMAGE_H
#define _RENDERER_WSAIMAGE_H

#include "../freecnc.h"

struct WSAError : public std::runtime_error
{
    WSAError(const string& msg) : std::runtime_error(msg) { }
};

class WSA
{
public:
    WSA(const string& fname);
    void animate();
private:
    SDL_Surface* decode_frame(unsigned short framenum);
    vector<unsigned char> wsadata, framedata;
    SDL_Color palette[256];
    bool loop;
    string sndfile;

    unsigned short c_frames;
    unsigned short xpos;
    unsigned short ypos;
    unsigned short width;
    unsigned short height;
    unsigned int delta;
    vector<unsigned int> offsets;
};

#endif
