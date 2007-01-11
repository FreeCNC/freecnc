#ifndef _RENDERER_SHPIMAGE_H
#define _RENDERER_SHPIMAGE_H

#include "SDL.h"
#include "../freecnc.h"

class ImageProc;
class VFile;

struct SHPHeader {
    unsigned short  NumImages;
    unsigned short  Width;
    unsigned short  Height;
    unsigned int* Offset;
    unsigned char*  Format;
    unsigned int* RefOffs;
    unsigned char*  RefFormat;
};

struct Dune2Header {
    unsigned short compression;
    unsigned char  cy;
    unsigned short cx;
    unsigned char  cy2;
    unsigned short size_in;
    unsigned short size_out;
};

//-----------------------------------------------------------------------------
// SHPBase
//-----------------------------------------------------------------------------

class SHPBase {
public:
    SHPBase(const std::string& fname, char scaleq = -1);
    virtual ~SHPBase();

    static void setPalette(SDL_Color *pal);
    static void calculatePalettes();
    static SDL_Color* getPalette(unsigned char palnum) { return palette[palnum]; }
    static unsigned int getColour(SDL_PixelFormat* fmt, unsigned char palnum, unsigned short index);
    static unsigned char numPalettes() { return numpals; }

    SDL_Surface* scale(SDL_Surface *input, int quality);
    const std::string& getName() const {return name;}

protected:
    static SDL_Color palette[32][256];
    static const unsigned char numpals;

    enum headerformats {FORMAT_20 = 0x20, FORMAT_40 = 0x40, FORMAT_80 = 0x80};
    std::string name;
    char scaleq;
    ImageProc* scaler;
};

inline unsigned int SHPBase::getColour(SDL_PixelFormat* fmt, unsigned char palnum, unsigned short index) {
    SDL_Color p = palette[palnum][index];
    return SDL_MapRGB(fmt, p.r, p.g, p.b);
}


//-----------------------------------------------------------------------------
// SHPImage
//-----------------------------------------------------------------------------

class SHPImage : SHPBase {
public:
    SHPImage(const char *fname, char scaleq);
    ~SHPImage();

    void getImage(unsigned short imgnum, SDL_Surface **img, SDL_Surface **shadow, unsigned char palnum);
    void getImageAsAlpha(unsigned short imgnum, SDL_Surface **img);

    unsigned int getWidth() const { return header.Width; }
    unsigned int getHeight() const { return header.Height; }
    unsigned short getNumImg() const { return header.NumImages; }

private:
    static SDL_Color shadowpal[2];
    static SDL_Color alphapal[6];

    void DecodeSprite(unsigned char *imgdst, unsigned short imgnum);
    unsigned char *shpdata;
    SHPHeader header;
};

//-----------------------------------------------------------------------------
// Dune2Image
//-----------------------------------------------------------------------------

class Dune2Image : SHPBase
{
public:
    Dune2Image(const char *fname, char scaleq);
    ~Dune2Image();

    SDL_Surface* getImage(unsigned short imgnum);

private:
    unsigned int getD2Header(unsigned short imgnum);
    unsigned char* shpdata;
    Dune2Header header;
};

//-----------------------------------------------------------------------------
// TemplateImage
//-----------------------------------------------------------------------------

class TemplateImage : SHPBase
{
public:
    TemplateImage(const char *fname, char scaleq, bool ratemp = false);
    ~TemplateImage();

    unsigned short getNumTiles();
    SDL_Surface* getImage(unsigned short imgnum);

private:
    bool ratemp;
    VFile* tmpfile;
};

struct ImageNotFound : public std::runtime_error
{
    ImageNotFound(const string& msg) : std::runtime_error(msg) {}
};

#endif
