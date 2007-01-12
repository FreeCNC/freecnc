#include <cstring>
#include <stdexcept>

#include "font.h"
#include "../lib/fcncendian.h"

using std::runtime_error;

namespace
{
    SDL_Color font_colours[] = {{0x0, 0x0, 0x0, 0x0}, {0xff, 0xff, 0xff, 0xff}};
}

Font::Font(const string& fontname) : SHPBase(fontname), fontimg(0)  {
    this->fontname = fontname;
    reload();
}

Font::~Font() {
    SDL_FreeSurface(fontimg);
}

void Font::reload() {
    shared_ptr<File> fontfile = game.vfs.open(fontname);

    if (!fontfile) {
        throw(runtime_error("Font: Unable to open '" + fontname + "'"));
    }

    fontfile->seek_start(8);
    vector<unsigned char> data(12);
    fontfile->read(data, 12);
    unsigned short wpos  = readword(data, 0);
    //unsigned short cdata = readword(data, 2);
    unsigned short hpos  = readword(data, 4);
    unsigned short nchars = readword(data, 8);
    // Have to swap on LE for some reason
    nchars = swap_endian(nchars);
    unsigned char fnheight = readbyte(data, 10);
    //unsigned char fnmaxw   = readbyte(data, 11);

    nchars++;

    vector<unsigned char> wchar(nchars);
    vector<unsigned char> hchar(nchars<<1);

    // Can't read directly to unsigned shorts yet...
    vector<unsigned char> tmp_offsets(nchars * 2);
    fontfile->read(tmp_offsets, nchars * 2);

    vector<unsigned short> dataoffsets(nchars);
    for (unsigned short i = 0; i < nchars; ++i)
    {
        dataoffsets[i] = readword(tmp_offsets, 2*i);
    }

    fontfile->seek_start(wpos);
    fontfile->read(wchar, nchars);
    fontfile->seek_start(hpos);
    fontfile->read(hchar, nchars<<1);

    chrdest.resize(nchars);

    unsigned int fntotalw = 0;
    for (int i = 0 ; i < nchars; ++i) {
        chrdest[i].x = fntotalw;
        chrdest[i].y = 0;
        chrdest[i].h = fnheight;
        chrdest[i].w = wchar[i];
        fntotalw += wchar[i];
    }
    vector<unsigned char> chardata(fnheight * fntotalw);

    for (int curchar = 0; curchar < nchars; ++curchar) {
        fontfile->seek_start(dataoffsets[curchar]);
        for (unsigned int ypos = hchar[curchar<<1]; ypos < (unsigned int)(hchar[curchar<<1] + hchar[(curchar<<1)+1]); ++ypos) {
            unsigned int pos = chrdest[curchar].x+ypos*fntotalw;
            for (unsigned int i = 0; i < wchar[curchar]; i+=2) {
                fontfile->read(data, 1);
                chardata[pos + i] = (data[0] & 0xf) != 0 ? 1 : 0;
                if (i + 1 < wchar[curchar])
                    chardata[pos + i + 1] = (data[0]>>4) != 0 ? 1 : 0;
            }
        }
    }

    SDL_FreeSurface(fontimg);
    fontimg = NULL;

    SDL_Surface *imgtmp = SDL_CreateRGBSurfaceFrom(&chardata[0], fntotalw,
        fnheight, 8, fntotalw, 0, 0, 0, 0);
    //
    //   SDL_SetColors(imgtmp, palette[0], 0, 256);
    SDL_SetColors(imgtmp, font_colours, 0, 2);
    SDL_SetColorKey(imgtmp, SDL_SRCCOLORKEY, 0);

    fontimg = SDL_DisplayFormat(imgtmp);
    SDL_FreeSurface(imgtmp);
}

unsigned int Font::getHeight() const {
    return chrdest[0].h;
}

unsigned int Font::calcTextWidth(const std::string& text) const {
    unsigned int wdt = 0;
    unsigned int i;

    for (i = 0; text[i] != '\0'; i++) {
        wdt += chrdest[text[i]].w+1;
    }
    return wdt;
}

void Font::drawText(const std::string& text, SDL_Surface *dest, unsigned int startx, unsigned int starty) const {
    unsigned int i;
    SDL_Rect destr;
    destr.x = startx;
    destr.y = starty;
    destr.h = chrdest[0].h;

    for (i = 0; text[i] != '\0'; i++) {
        SDL_Rect* src_rect = const_cast<SDL_Rect*>(&chrdest[text[i]]);
        destr.w = src_rect->w;
        SDL_BlitSurface(fontimg, src_rect, dest, &destr);
        destr.x += destr.w+1;
    }
}
