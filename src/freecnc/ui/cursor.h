#ifndef _UI_CURSOR_H
#define _UI_CURSOR_H

#include "../freecnc.h"

struct SDL_Surface;

struct CursorInfo;
class CursorPool;
class Dune2Image;
class TemplateImage;

/// @TODO Move these hardcoded values to inifile.
#define MAX_CURS_IN_ANIM 24

#define CUR_NOANIM 1

#define CUR_STANDARD 0
#define CUR_SCROLLUP 1
#define CUR_SCROLLUR 2
#define CUR_SCROLLRIGHT 3
#define CUR_SCROLLDR 4
#define CUR_SCROLLDOWN 5
#define CUR_SCROLLDL 6
#define CUR_SCROLLLEFT 7
#define CUR_SCROLLUL 8
#define CUR_NOSCROLL_OFFSET 129
#define CUR_RA_NOSCROLL_OFFSET 123

#define CUR_PLACE 250

#define MAXCURNAME 12

class Cursor
{
public:
    Cursor();
    ~Cursor();
    void setCursor(unsigned short cursornum, unsigned char animimages);
    void setCursor(const char* curname);
    void setPlaceCursor(unsigned char stw, unsigned char sth, unsigned char *icn);
    SDL_Surface *getCursor();/*{return image[curimg];}*/

    unsigned short getX()
    {
        return x+cursor_offset;
    }
    unsigned short getY()
    {
        return y+cursor_offset;
    }
    void setXY(unsigned short nx, unsigned short ny)
    {
        x = nx;
        y = ny;
    }
    static unsigned char getNoScrollOffset() {
        return nsoff;
    }
    //   void setY(unsigned short ny){y = ny;}
    void reloadImages();
private:
    unsigned short currentcursor;

    unsigned short x, y;

    unsigned char curimg;
    unsigned char nimgs;

    // Either CUR_RA_NOSCROLL_OFFSET or CUR_NOSCROLL_OFFSET
    static unsigned char nsoff;

    SDL_Surface *image[MAX_CURS_IN_ANIM];

    Dune2Image *cursorimg;
    TemplateImage *transicn;
    short cursor_offset;

    CursorPool* cursorpool;
    CursorInfo* ci;
    SDL_Surface* transw, *transy, *transr;

    //Used by place cursor
    unsigned int oldptr;
};

#endif
