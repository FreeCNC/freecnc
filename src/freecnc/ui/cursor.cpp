#include <cstdlib>
#include <cstring>

#include "SDL.h"

#include "../renderer/renderer_public.h"
#include "cursor.h"
#include "cursorpool.h"

Uint8 Cursor::nsoff;

/** Constructor, load the cursorimage and set upp all surfaces to buffer
 * background in etc.
 */
Cursor::Cursor() : currentcursor(0xffff), x(0), y(0), curimg(0), nimgs(0), ci(0),
                   transw(0), transy(0), transr(0)
{
    cursorimg = new Dune2Image("mouse.shp", -1);
    if (getConfig().gamenum == GAME_RA) {
        transicn = new TemplateImage("trans.icn", mapscaleq, 1);
        nsoff = CUR_RA_NOSCROLL_OFFSET;
    } else {
        transicn = new TemplateImage("trans.icn", mapscaleq);
        nsoff = CUR_NOSCROLL_OFFSET;
    }

    cursorpool = new CursorPool("cursors.ini");

    // Load the first simple cursor
    setCursor("STANDARD");
    x = 0; y = 0;

    /* All cursors loaded */
    reloadImages();
}

/** Destructor, free the surfaces */
Cursor::~Cursor()
{
    for (Uint8 i = 0; i < nimgs; ++i)
        SDL_FreeSurface(image[i]);

    SDL_FreeSurface(transw);
    SDL_FreeSurface(transy);
    SDL_FreeSurface(transr);

    delete cursorimg;
    delete transicn;
    delete cursorpool;
}

void Cursor::reloadImages() {
    int cursornum;

    //Free all the old images
    SDL_FreeSurface(transw); transw = NULL;
    SDL_FreeSurface(transy); transy = NULL;
    SDL_FreeSurface(transr); transr = NULL;

    //Reload stuff
    transw = transicn->getImage(0);
    transy = transicn->getImage(1);
    transr = transicn->getImage(2);

    //Reload cursor image array
    if (currentcursor != CUR_PLACE) {
        //If its not a place cursor, invalid current cursor, and reload it
        cursornum = currentcursor;
        currentcursor = 0xffff;
        setCursor(cursornum, nimgs);
    }
}

/** Change active cursor.
 * @param number of the new cursor.
 */
void Cursor::setCursor(Uint16 cursornum, Uint8 animimages)
{
    int i;

    //Don't load this cursor again
    if (currentcursor == cursornum)
        return;

    for (i = 0; i < nimgs; ++i)
        SDL_FreeSurface(image[i]);

    curimg = 0;
    nimgs = animimages;
    for (i = 0; i < nimgs; ++i)
        image[i] = cursorimg->getImage(cursornum+i);

    if (cursornum != CUR_STANDARD) {
        cursor_offset = -((image[0]->w)>>1);
    } else {
        cursor_offset = 0;
    }

    currentcursor = cursornum;
}

void Cursor::setCursor(const char* curname)
{
    ci = cursorpool->getCursorByName(curname);

    setCursor(ci->anstart,  ci->anend - ci->anstart + 1);
}

void Cursor::setPlaceCursor(Uint8 stw, Uint8 sth, Uint8 *icn)
{
    int i, x, y;
    SDL_Surface *bigimg;

    Uint8 *data;
    SDL_Rect dest;
    Uint32 newptr;

    newptr = 0;
    for(i = 0; i < stw*sth; i++) {
        newptr = newptr<<2|(icn[i]);
    }

    if (currentcursor == CUR_PLACE && newptr == oldptr) {
        return;
    }
    oldptr = newptr;

    for( i = 0; i < nimgs; i++ )
        SDL_FreeSurface( image[i] );

    curimg = 0;
    currentcursor = CUR_PLACE;
    nimgs = 1;
    cursor_offset = 0;

    data = new Uint8[stw*sth*transw->w*transw->h];
    memset(data, 0, stw*sth*transw->w*transw->h);

    bigimg = SDL_CreateRGBSurfaceFrom(data, stw*transw->w, sth*transw->h,
                                      8, stw*transw->w, 0, 0, 0, 0);

    SDL_SetColors(bigimg, SHPBase::getPalette(0), 0, 256);
    SDL_SetColorKey(bigimg, SDL_SRCCOLORKEY, 0);

    image[0] = SDL_DisplayFormat(bigimg);
    SDL_FreeSurface(bigimg);
    delete[] data;

    dest.w = transw->w;
    dest.h = transw->h;

    dest.y = 0;
    for( y = 0; y < sth; y++ ) {
        dest.x = 0;
        for( x = 0; x < stw; x++ ) {
            SDL_Surface** tile = 0;
            if (icn[y*stw+x] == 0) {
                dest.x += dest.w;
                continue;
            }
            switch (icn[y*stw+x]) {
            case 1:
                tile = &transw;
                break;
            case 2:
                tile = &transy;
                break;
            case 4:
                tile = &transr;
                break;
            default:
                logger->error("Possible memory corruption detected in %s: icn[%i*%i+%i] = %i\n",__FUNCTION__,y,stw,x,icn[y*stw+x]);
                //throw InvalidValue();
                throw 0;
                break;
            }
            SDL_BlitSurface(*tile, NULL, image[0], &dest);
            dest.x += dest.w;
        }
        dest.y += dest.h;
    }
}

SDL_Surface *Cursor::getCursor()
{
    static Uint32 lastchange = 0;
    Uint32 tick = SDL_GetTicks();
    if( tick > lastchange +100 ) {
        curimg++;
        if( curimg >= nimgs )
            curimg = 0;
        lastchange = tick;
    }
    return image[curimg];
}
