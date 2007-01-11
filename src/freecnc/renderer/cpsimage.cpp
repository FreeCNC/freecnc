#include <cstdlib>
#include <cstring>

#include "../lib/compression.h"
#include "../lib/inifile.h"
#include "shpimage.h"
#include "cpsimage.h"
#include "../lib/fcncendian.h"

CPSImage::CPSImage(const char* fname, int scaleq) : image(0)
{
    this->scaleq = scaleq;
    shared_ptr<File> imgfile = game.vfs.open(fname);
    if (!imgfile) {
        throw ImageNotFound("CPS loader: " + string(fname) + " not found");
    }

    imgsize = imgfile->size();
    cpsdata.resize(imgsize);
    imgfile->read(cpsdata, imgfile->size());

    header.size    = readword(cpsdata,0);
    header.unknown = readword(cpsdata,2);
    header.imsize  = readword(cpsdata,4);
    header.palette = readlong(cpsdata,6);
    if (header.palette == 0x3000000) {
        readPalette();
    } else {
        // magic here to select appropriate palette
        offset = 10;
    }
}

CPSImage::~CPSImage()
{
    SDL_FreeSurface(image);
}

void CPSImage::readPalette()
{
    unsigned short i;
    offset = 10;
    for (i = 0; i < 256; i++) {
        palette[i].r = readbyte(cpsdata, offset);
        palette[i].g = readbyte(cpsdata, offset+1);
        palette[i].b = readbyte(cpsdata, offset+2);
        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
        offset += 3;
    }
}

SDL_Surface* CPSImage::getImage()
{
    if (!image) {
        loadImage();
    }
    return image;
}

void CPSImage::loadImage()
{
//    unsigned int len = imgsize - offset;

    vector<unsigned char> image_data(header.imsize);
    Compression::decode80(&cpsdata[0] + offset, &image_data[0]);

    SDL_Surface* imgtmp = SDL_CreateRGBSurfaceFrom(&image_data[0], 320, 200, 8,
        320, 0, 0, 0, 0);
    SDL_SetColors(imgtmp, palette, 0, 256);
    SDL_SetColorKey(imgtmp, SDL_SRCCOLORKEY, 0);

    vector<unsigned char>().swap(cpsdata);

    if (scaleq >= 0) {
        image = scaler.scale(imgtmp, scaleq);
        SDL_SetColorKey(image, SDL_SRCCOLORKEY, 0);
    } else {
        image = SDL_DisplayFormat(imgtmp);
    }
    SDL_FreeSurface(imgtmp);
}
