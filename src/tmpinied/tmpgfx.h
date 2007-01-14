#ifndef TMPGFX_H
#define TMPGFX_H

class TemplateData;
class TemplateImage;
class Font;
struct SDL_Surface;

class TmpGFX
{
public:
    TmpGFX();
    ~TmpGFX();
    void draw(TemplateData* data, TemplateImage* marker, Font* font, unsigned int curpos);
private:
    SDL_Surface* screen;
};

#endif
