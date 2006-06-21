#ifndef _UI_FONT_H
#define _UI_FONT_H

#include "SDL.h"

#include "../freecnc.h"
#include "../renderer/shpimage.h"

class Font : SHPBase {
public:
    Font(const std::string& fontname);
    ~Font();
    unsigned int getHeight() const;
    unsigned int calcTextWidth(const string& text) const;
    void drawText(const string& text, SDL_Surface* dest, unsigned int startx, unsigned int starty) const;
    void reload();
private:
    SDL_Surface* fontimg;
    vector<SDL_Rect> chrdest;
    string fontname;
};

#endif
