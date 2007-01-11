#ifndef _RENDERER_CPSIMAGE_H
#define _RENDERER_CPSIMAGE_H

#include "SDL.h"

#include "../freecnc.h"
#include "imageproc.h"

struct CPSHeader
{
    unsigned short size;
    unsigned short unknown;
    unsigned short imsize;
    unsigned int palette;
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

    unsigned int imgsize, offset;
    vector<unsigned char> cpsdata;

    SDL_Color palette[256];
    CPSHeader header;
    int scaleq;
    ImageProc scaler;
    SDL_Surface* image;
};

#endif
