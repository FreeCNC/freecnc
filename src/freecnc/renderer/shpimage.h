#ifndef _RENDERER_SHPIMAGE_H
#define _RENDERER_SHPIMAGE_H

#include "SDL.h"
#include "../freecnc.h"

class ImageProc;
class VFile;

struct SHPHeader {
    Uint16  NumImages;
    Uint16  Width;
    Uint16  Height;
    Uint32* Offset;
    Uint8*  Format;
    Uint32* RefOffs;
    Uint8*  RefFormat;
};

struct Dune2Header {
    Uint16 compression;
    Uint8  cy;
    Uint16 cx;
    Uint8  cy2;
    Uint16 size_in;
    Uint16 size_out;
};

//-----------------------------------------------------------------------------
// SHPBase
//-----------------------------------------------------------------------------

class SHPBase {
public:
    SHPBase(const std::string& fname, Sint8 scaleq = -1);
    virtual ~SHPBase();

    static void setPalette(SDL_Color *pal);
    static void calculatePalettes();
    static SDL_Color* getPalette(Uint8 palnum) { return palette[palnum]; }
    static Uint32 getColour(SDL_PixelFormat* fmt, Uint8 palnum, Uint16 index);
    static Uint8 numPalettes() { return numpals; }

    SDL_Surface* scale(SDL_Surface *input, int quality);
    const std::string& getName() const {return name;}

protected:
    static SDL_Color palette[32][256];
    static const Uint8 numpals;

    enum headerformats {FORMAT_20 = 0x20, FORMAT_40 = 0x40, FORMAT_80 = 0x80};
    std::string name;
    Sint8 scaleq;
    ImageProc* scaler;
};

inline Uint32 SHPBase::getColour(SDL_PixelFormat* fmt, Uint8 palnum, Uint16 index) {
    SDL_Color p = palette[palnum][index];
    return SDL_MapRGB(fmt, p.r, p.g, p.b);
}


//-----------------------------------------------------------------------------
// SHPImage
//-----------------------------------------------------------------------------

class SHPImage : SHPBase {
public:
    SHPImage(const char *fname, Sint8 scaleq);
    ~SHPImage();

    void getImage(Uint16 imgnum, SDL_Surface **img, SDL_Surface **shadow, Uint8 palnum);
    void getImageAsAlpha(Uint16 imgnum, SDL_Surface **img);

    Uint32 getWidth() const { return header.Width; }
    Uint32 getHeight() const { return header.Height; }
    Uint16 getNumImg() const { return header.NumImages; }

private:
    static SDL_Color shadowpal[2];
    static SDL_Color alphapal[6];

    void DecodeSprite(Uint8 *imgdst, Uint16 imgnum);
    Uint8 *shpdata;
    SHPHeader header;
};

//-----------------------------------------------------------------------------
// Dune2Image
//-----------------------------------------------------------------------------

class Dune2Image : SHPBase
{
public:
    Dune2Image(const char *fname, Sint8 scaleq);
    ~Dune2Image();

    SDL_Surface* getImage(Uint16 imgnum);

private:
    Uint32 getD2Header(Uint16 imgnum);
    Uint8* shpdata;
    Dune2Header header;
};

//-----------------------------------------------------------------------------
// TemplateImage
//-----------------------------------------------------------------------------

class TemplateImage : SHPBase
{
public:
    TemplateImage(const char *fname, Sint8 scaleq, bool ratemp = false);
    ~TemplateImage();

    Uint16 getNumTiles();
    SDL_Surface* getImage(Uint16 imgnum);

private:
    bool ratemp;
    VFile* tmpfile;
};

class ImageNotFound {};

#endif
