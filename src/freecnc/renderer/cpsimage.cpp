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

    vector<unsigned char>::iterator it = cpsdata.begin();
    header.size    = read_word(it, FCNC_LIL_ENDIAN);
    header.unknown = read_word(it, FCNC_LIL_ENDIAN);
    header.imsize  = read_word(it, FCNC_LIL_ENDIAN);
    header.palette = read_dword(it, FCNC_LIL_ENDIAN);
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
    vector<unsigned char>::iterator it = cpsdata.begin(); it += 10;
    for (int i = 0; i < 256; i++) {
        palette[i].r = read_byte(it) << 2;
        palette[i].g = read_byte(it) << 2;
        palette[i].b = read_byte(it) << 2;
    }
    offset = 10 + 3*256;
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
    Compression::decode80(&cpsdata[offset], &image_data[0]);

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
