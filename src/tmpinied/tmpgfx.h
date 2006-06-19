#ifndef TMPGFX_H
#define TMPGFX_H

#include "SDL.h"
#include "templatedata.h"
#include "font.h"
#include "shpimage.h"

class TmpGFX
{
public:
    TmpGFX();
    ~TmpGFX();
    void draw(TemplateData *data, TemplateImage *trans, Font *fnt, Uint32 curpos);
private:
    SDL_Surface *screen;
};

#endif
