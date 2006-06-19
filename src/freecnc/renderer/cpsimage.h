#ifndef _RENDERER_CPSIMAGE_H
#define _RENDERER_CPSIMAGE_H

#include "SDL.h"

#include "../freecnc.h"
#include "imageproc.h"

struct CPSHeader
{
    Uint16 size;
    Uint16 unknown;
    Uint16 imsize;
    Uint32 palette;
};

class CPSImage
{
public:
    CPSImage(const char* fname, int scaleq);
    ~CPSImage();
    SDL_Surface* getImage();
private:
    void loadImage();
    void readPalette();
    Uint32 imgsize, offset;
    Uint8* cpsdata;
    SDL_Color palette[256];
    CPSHeader header;
    int scaleq;
    ImageProc scaler;
    SDL_Surface* image;
};

#endif
