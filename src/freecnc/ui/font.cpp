#include <cstring>
#include <stdexcept>

#include "../vfs/vfs_public.h"
#include "font.h"

using std::runtime_error;

Font::Font(const string& fontname) : SHPBase(fontname), fontimg(0)  {
    this->fontname = fontname;
    reload();
}

Font::~Font() {
    SDL_FreeSurface(fontimg);
}

void Font::reload() {
    VFile *fontfile;
    Uint16 wpos, hpos, cdata, nchars;
    Uint8 fnheight, fnmaxw;
    Uint32 fntotalw;
    //vector<Uint8> chardata, wchar, hchar;
    Uint32 ypos, i, pos, curchar;
    Uint8 data;
    //Uint16 *dataoffsets;
    SDL_Surface *imgtmp;

    static SDL_Color colours[] = {{0x0, 0x0, 0x0, 0x0}, {0xff, 0xff, 0xff, 0xff}};
    fontfile = VFS_Open(fontname.c_str());

    if (0 == fontfile) {
        string s("Unable to load font ");
        s += fontname;
        throw(runtime_error(s));
    }

    fontfile->seekSet(8);
    fontfile->readWord(&wpos, 1);
    fontfile->readWord(&cdata, 1);
    fontfile->readWord(&hpos, 1);
    fontfile->seekCur(2);
    fontfile->readWord(&nchars, 1);
    nchars = SDL_Swap16(nchars); // Yes, even on LE systems.
    fontfile->readByte(&fnheight, 1);
    fontfile->readByte(&fnmaxw, 1);

    nchars++;

    vector<Uint8> wchar(nchars);
    vector<Uint8> hchar(nchars<<1);

    vector<Uint16> dataoffsets(nchars);
    fontfile->readWord(&dataoffsets[0], nchars);

    fontfile->seekSet(wpos);
    fontfile->readByte(&wchar[0], nchars);
    fontfile->seekSet(hpos);
    fontfile->readByte(&hchar[0], nchars<<1);

    chrdest.resize(nchars);

    fntotalw = 0;
    for( i = 0 ; i<nchars; i++ ) {
        chrdest[i].x = fntotalw;

        chrdest[i].y = 0;
        chrdest[i].h = fnheight;
        chrdest[i].w = wchar[i];
        fntotalw += wchar[i];
    }
    vector<Uint8> chardata(fnheight*fntotalw);

    for( curchar = 0; curchar < nchars; curchar++ ) {
        fontfile->seekSet(dataoffsets[curchar]);
        for( ypos = hchar[curchar<<1]; ypos < (Uint32)(hchar[curchar<<1]+hchar[(curchar<<1)+1]); ypos++ ) {
            pos = chrdest[curchar].x+ypos*fntotalw;
            for( i = 0; i < wchar[curchar]; i+=2 ) {
                fontfile->readByte( &data, 1 );
                chardata[pos+i] = (data&0xf)!=0?1:0;
                if( i+1<wchar[curchar] )
                    chardata[pos+i+1] = (data>>4)!=0?1:0;
            }
        }
    }

    SDL_FreeSurface(fontimg);
    fontimg = NULL;

    imgtmp = SDL_CreateRGBSurfaceFrom(&chardata[0], fntotalw, fnheight,
                                      8, fntotalw, 0, 0, 0, 0);
    //   SDL_SetColors(imgtmp, palette[0], 0, 256);
    SDL_SetColors(imgtmp, colours, 0, 2);
    SDL_SetColorKey(imgtmp, SDL_SRCCOLORKEY, 0);

    fontimg = SDL_DisplayFormat(imgtmp);
    SDL_FreeSurface(imgtmp);

    VFS_Close(fontfile);
}

Uint32 Font::getHeight() const {
    return chrdest[0].h;
}

Uint32 Font::calcTextWidth(const std::string& text) const {
    Uint32 wdt = 0;
    Uint32 i;

    for (i = 0; text[i] != '\0'; i++)
        wdt += chrdest[text[i]].w+1;
    return wdt;
}

void Font::drawText(const std::string& text, SDL_Surface *dest, Uint32 startx, Uint32 starty) const {
    Uint32 i;
    SDL_Rect destr;
    destr.x = startx;
    destr.y = starty;
    destr.h = chrdest[0].h;

    for( i = 0; text[i] != '\0'; i++ ) {
        SDL_Rect* src_rect = const_cast<SDL_Rect*>(&chrdest[text[i]]);
        destr.w = src_rect->w;
        SDL_BlitSurface(fontimg, src_rect, dest, &destr);
        destr.x += destr.w+1;
    }
}
