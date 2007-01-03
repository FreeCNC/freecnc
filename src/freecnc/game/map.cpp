#include <cmath>
#include <iostream>

#include "../renderer/renderer_public.h"
#include "map.h"
#include "playerpool.h"
#include "structure.h"
#include "unit.h"
#include "unitandstructurepool.h"

//-----------------------------------------------------------------------------
// Functors
//-----------------------------------------------------------------------------

struct TemplateCacheCleaner : public std::unary_function<TemplateCache::value_type, void>
{
    void operator()(const TemplateCache::value_type& p)
    {
        delete p.second;
    }
};

struct TemplateTileCacheCleaner : public std::unary_function<TemplateTileCache::value_type, void>
{
    void operator()(const TemplateTileCache::value_type& p)
    {
        delete p;
    }
};

//-----------------------------------------------------------------------------
// CnCMap Methods
//-----------------------------------------------------------------------------

unsigned int CnCMap::getOverlay(unsigned int pos)
{
    if (overlaymatrix[pos] & HAS_OVERLAY)
        return overlays[pos]<<16;
    return 0;
}

unsigned int CnCMap::getTerrain(unsigned int pos, short* xoff, short* yoff)
{
    unsigned int terrain = 0;

    if (overlaymatrix[pos] & HAS_TERRAIN) {
        terrain = terrains[pos].shpnum;
        *xoff = terrains[pos].xoffset;
        *yoff = terrains[pos].yoffset;
    }
    return terrain;
}

/** @brief Sets up things that don't depend on the map being loaded.
 */
CnCMap::CnCMap()
{
    loaded  = false;
    pc::imagepool = new std::vector<SHPImage*>();
    pc::imgcache->setImagePool(pc::imagepool);
    this->maptype = game.config.gametype;
    scrollstep = game.config.scrollstep;
    scrolltime = game.config.scrolltime;
    maxscroll  = game.config.maxscroll;
    /* start at top right corner of map. */
    // the startpos for the map is stored in position 0
    scrollpos.curx = 0;
    scrollpos.cury = 0;
    scrollpos.curxtileoffs = 0;
    scrollpos.curytileoffs = 0;

    scrollvec.x = 0;
    scrollvec.y = 0;
    scrollvec.t = 0;
    toscroll    = false;
    for (unsigned char i=0;i<NUMMARKS;++i) {
        scrollbookmarks[i].x = 0;
        scrollbookmarks[i].y = 0;
        scrollbookmarks[i].xtile = 0;
        scrollbookmarks[i].ytile = 0;
    }
    minimap = NULL;
    oldmmap = NULL;
    loading = false;
    translate_64 = (game.config.gametype == GAME_TD);
}

/** Destructor, free up some memory */
CnCMap::~CnCMap()
{
    unsigned int i;

    for( i = 0; i < tileimages.size(); i++ )
        SDL_FreeSurface(tileimages[i]);

    for( i = 0; i < pc::imagepool->size(); i++ )
        delete (*pc::imagepool)[i];

    // Empty the cache of TemplateImage* & Tile
    for_each(templateTileCache.begin(), templateTileCache.end(), TemplateTileCacheCleaner());

    // Empty the pool of TemplateImage*
    for_each(templateCache.begin(), templateCache.end(), TemplateCacheCleaner());

    delete p::uspool;
    delete p::ppool;
    SDL_FreeSurface(minimap);
    for( i = 0; i < numShadowImg; i++ ) {
        SDL_FreeSurface(shadowimages[i]);
    }
    delete pc::imagepool;
}

/** @TODO Map loading goes here.
 */
void CnCMap::loadMap(const char* mapname, LoadingScreen* lscreen) {
    missionData.mapname = mapname;
    string message = "Reading "; message += mapname;
    message += ".INI";
    loading = true;
    /* Load the ini part of the map */
    lscreen->setCurrentTask(message);
    loadIni();

    /* Load the bin part of the map (the tiles) */
    message = "Loading "; message += missionData.mapname;
    message += ".BIN";
    lscreen->setCurrentTask(message);
    if (maptype == GAME_TD)
        loadBin();

    //   Path::setMapSize(width, height);
    p::ppool->setAlliances();

    loading = false;
    loaded = true;
}

/** sets the scroll to the specified direction.
 * @param direction to scroll in.
 */
unsigned char CnCMap::accScroll(unsigned char direction)
{
    bool validx = false, validy = false;
    if (direction & s_up) {
        if (scrollvec.y >= 0)
            scrollvec.y = -scrollstep;
        else if (scrollvec.y > -maxscroll)
            scrollvec.y -= scrollstep;
        validy = (valscroll & s_up);
        if (!validy) {
            scrollvec.y = 0;
            direction ^= s_up;
        }
    }
    if (direction & s_down) {
        if (scrollvec.y <= 0)
            scrollvec.y = scrollstep;
        else if (scrollvec.y < maxscroll)
            scrollvec.y += scrollstep;
        validy = (valscroll & s_down);
        if (!validy) {
            scrollvec.y = 0;
            direction ^= s_down;
        }
    }
    if (direction & s_left) {
        if (scrollvec.x >= 0)
            scrollvec.x =  -scrollstep;
        else if (scrollvec.x > -maxscroll)
            scrollvec.x -= scrollstep;
        validx = (valscroll & s_left);
        if (!validx) {
            scrollvec.x = 0;
            direction ^= s_left;
        }
    }
    if (direction & s_right) {
        if (scrollvec.x <= 0)
            scrollvec.x = scrollstep;
        else if (scrollvec.x < maxscroll)
            scrollvec.x += scrollstep;
        validx = (valscroll & s_right);
        if (!validx) {
            scrollvec.x = 0;
            direction ^= s_right;
        }
    }
    if (validx || validy) {
        scrollvec.t = 0;
        toscroll = true;
    }
    return direction;
}

unsigned char CnCMap::absScroll(short dx, short dy, unsigned char border)
{
    static const double fmax = (double)maxscroll/100.0;
    unsigned char direction = s_none;
    bool validx = false, validy = false;
    if (dx <= -border) {
        validx = (valscroll & s_left);
        if (validx) {
            scrollvec.x = (char)(min(dx,(short)100) * fmax);
            direction |= s_left;
        } else {
            scrollvec.x = 0;
        }
    } else if (dx >= border) {
        validx = (valscroll & s_right);
        if (validx) {
            scrollvec.x = (char)(min(dx,(short)100) * fmax);
            direction |= s_right;
        } else {
            scrollvec.x = 0;
        }
    }
    if (dy <= -border) {
        validy = (valscroll & s_up);
        if (validy) {
            scrollvec.y = (char)(min(dy,(short)100) * fmax);
            direction |= s_up;
        } else {
            scrollvec.y = 0;
        }
    } else if (dy >= border) {
        validy = (valscroll & s_down);
        if (validy) {
            scrollvec.y = (char)(min(dy,(short)100) * fmax);
            direction |= s_down;
        } else {
            scrollvec.y = 0;
        }
    }
    toscroll = (validx || validy);
    return direction;
}


/** scrolls according to the scroll vector.
*/
void CnCMap::doscroll()
{
    int xtile, ytile;
    if( scrollpos.curx*scrollpos.tilewidth+scrollpos.curxtileoffs <= -scrollvec.x &&
            scrollvec.x < 0) {
        scrollvec.t = 0;
        scrollvec.x = 0;
        scrollpos.curx = 0;
        scrollpos.curxtileoffs = 0;
    }
    if( scrollpos.cury*scrollpos.tilewidth+scrollpos.curytileoffs <= -scrollvec.y &&
            scrollvec.y < 0) {
        scrollvec.t = 0;
        scrollvec.y = 0;
        scrollpos.cury = 0;
        scrollpos.curytileoffs = 0;
    }
    if( scrollpos.curx*scrollpos.tilewidth+scrollpos.curxtileoffs+scrollvec.x >=
            scrollpos.maxx*scrollpos.tilewidth+scrollpos.maxxtileoffs &&
            scrollvec.x > 0) {
        scrollvec.t = 0;
        scrollvec.x = 0;
        scrollpos.curx = scrollpos.maxx;
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    }
    if( scrollpos.cury*scrollpos.tilewidth+scrollpos.curytileoffs+scrollvec.y >=
            scrollpos.maxy*scrollpos.tilewidth+scrollpos.maxytileoffs &&
            scrollvec.y > 0) {
        scrollvec.t = 0;
        scrollvec.y = 0;
        scrollpos.cury = scrollpos.maxy;
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    }

    if ((scrollvec.x == 0) && (scrollvec.y == 0)) {
        toscroll = false;
        setValidScroll();
        return;
    }
    xtile = scrollpos.curxtileoffs+scrollvec.x;
    while( xtile < 0 ) {
        scrollpos.curx--;
        xtile += scrollpos.tilewidth;
    }
    while( xtile >= scrollpos.tilewidth ) {
        scrollpos.curx++;
        xtile -= scrollpos.tilewidth;
    }
    scrollpos.curxtileoffs = xtile;

    ytile = scrollpos.curytileoffs+scrollvec.y;
    while( ytile < 0 ) {
        scrollpos.cury--;
        ytile += scrollpos.tilewidth;
    }
    while( ytile >= scrollpos.tilewidth ) {
        scrollpos.cury++;
        ytile -= scrollpos.tilewidth;
    }
    scrollpos.curytileoffs = ytile;


    ++scrollvec.t;
    /* scrolling continues at current rate for scrolltime
     * passes then decays quickly */
    if (scrollvec.t >= scrolltime) {
        scrollvec.x /=2;
        scrollvec.y /=2;
    }
    setValidScroll();
}



/** sets the maximum value the scroll can take on.
 * @param the maximum x scroll.
 * @param the maximum y scroll.
 */

void CnCMap::setMaxScroll( unsigned int x, unsigned int y, unsigned int xtile, unsigned int ytile, unsigned int tilew )
{
    scrollpos.maxx = 0;
    scrollpos.maxy = 0;
    scrollpos.maxxtileoffs = 0;
    scrollpos.maxytileoffs = 0;
    if( xtile > 0 ) {
        x++;
        xtile = tilew-xtile;
    }
    if( ytile > 0 ) {
        y++;
        ytile = tilew-ytile;
    }

    if( width > x ) {
        scrollpos.maxx = width - x;
        scrollpos.maxxtileoffs = xtile;
    }
    if( height > y ) {
        scrollpos.maxy = height - y;
        scrollpos.maxytileoffs = ytile;
    }

    scrollpos.tilewidth = tilew;

    if( scrollpos.curx > scrollpos.maxx ) {
        scrollpos.curx = scrollpos.maxx;
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    } else if( scrollpos.curx == scrollpos.maxx &&
               scrollpos.curxtileoffs > scrollpos.maxxtileoffs ) {
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    }

    if( scrollpos.cury > scrollpos.maxy ) {
        scrollpos.cury = scrollpos.maxy;
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    } else if( scrollpos.cury == scrollpos.maxy &&
               scrollpos.curytileoffs > scrollpos.maxytileoffs ) {
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    }

    if( scrollpos.curxtileoffs > scrollpos.tilewidth ) {
        scrollpos.curxtileoffs = scrollpos.tilewidth;
    }
    if( scrollpos.curytileoffs > scrollpos.tilewidth ) {
        scrollpos.curytileoffs = scrollpos.tilewidth;
    }

    setValidScroll();
}

void CnCMap::setValidScroll()
{
    unsigned char temp = s_all;
    if( scrollpos.curx == 0 && scrollpos.curxtileoffs == 0 ) {
        temp &= ~s_left;
    }
    if( scrollpos.cury == 0 && scrollpos.curytileoffs == 0 ) {
        temp &= ~s_up;
    }
    if( scrollpos.curx == scrollpos.maxx &&
            scrollpos.curxtileoffs == scrollpos.maxxtileoffs ) {
        temp &= ~s_right;
    }
    if( scrollpos.cury == scrollpos.maxy &&
            scrollpos.curytileoffs == scrollpos.maxytileoffs ) {
        temp &= ~s_down;
    }
    valscroll = temp;
}

bool CnCMap::isBuildableAt(unsigned short pos, Unit* excpUn) const
{
    UnitOrStructure* uos = 0;
    // Can't build where you haven't explored
    if (!p::ppool->getLPlayer()->getMapVis()[pos]) {
        return false;
    }
    // Can't build on tiberium
    /* TODO
    if (getTiberium(pos) != 0) {
        return false;
    }*/
    switch (terraintypes[pos]) {
    case t_rock:
    case t_tree:
    case t_water_blocked:
    case t_other_nonpass:
        return false;
    case t_water:
        // Eventually will check if building is supposed to be in the water
        return false;
    case t_road:
    case t_land:
        if (p::uspool->tileAboutToBeUsed(pos)) {
            return false;
        }
        uos = p::uspool->getUnitOrStructureAt(pos,0x80,true);
        if (uos == excpUn)
            return true;
        if (uos == 0)
            return true;
        return false;
    default:
        return false;
    }
    // Unreachable
}

unsigned short CnCMap::getCost(unsigned short pos, Unit* excpUn) const
{
    unsigned short cost;

    if( !p::ppool->getLPlayer()->getMapVis()[pos] &&
            (excpUn == 0 || excpUn->getDist(pos)>1 )) {
        return 10;
    }

    /** @TODO: Tile cost should worked out as follows
     * if tmp == 1 then impassible
     * else unitspeed * tmp
     * where "tmp" is the terrain movement penalty.  This is a percentage of how much
     * of the unit's speed is lost when using this terrain type for the
     * unit's type of movement (foot, wheel, track, boat, air.
     * 
     * Unitspeed is used as it might be of use if more heuristics are used
     * when moving groups of units (e.g. either put slower moving units on
     * terrain that lets them move faster to get mixed units to stick
     * together or let faster moving units through a chokepoint first)
     */
    switch(terraintypes[pos]) {
    case t_rock:
    case t_tree:
    case t_water:
    case t_water_blocked:
    case t_other_nonpass:
        cost = 0xffff;
        break;
    case t_road:
        cost = p::uspool->getTileCost(pos,excpUn);
        break;
    case t_land:
    default:
        cost = 1 + p::uspool->getTileCost(pos,excpUn);
        break;
    }
    /** @TODO If unit prefers to be near tiberium (harvester) or should avoid
     * it at all costs (infantry except chem.) apply appropriate bonus/penatly
     * to cost.
     */
    return cost;
}

SDL_Surface* CnCMap::getMiniMap(unsigned char pixsize) {
    static ImageProc ip;
    unsigned short tx,ty;
    SDL_Rect pos = {0, 0, pixsize, pixsize};
    SDL_Surface *cminitile;
    if (pixsize == 0) {
        // Argh
        game.log << "CnCMap::getMiniMap: pixsize is zero, resetting to one" << endl;
        pixsize = 1;
    }
    if(minimap != NULL) {
        if (minimap->w == width*pixsize) {
            return minimap;
        } else {
            // Each minimap surface is about 250k, so caching a lot of zooms
            // would be somewhat expensive.  Could make how much memory to set aside
            // for this customizable so people with half a gig of RAM can see some
            // usage :-)
            // For now, just keep the previous one.
            SDL_FreeSurface(oldmmap);
            oldmmap = minimap;
            minimap = NULL;
        }
    }
    if (oldmmap != NULL && (oldmmap->w == pixsize*width)) {
        minimap = oldmmap;
        oldmmap = NULL;
        return minimap;
    }
    minimap = SDL_CreateRGBSurface (SDL_SWSURFACE, width*pixsize,height*pixsize, 16,
                                    0xff, 0xff, 0xff, 0);
    SDL_Surface *maptileOne = getMapTile(0);
    minimap->format->Rmask = maptileOne->format->Rmask;
    minimap->format->Gmask = maptileOne->format->Gmask;
    minimap->format->Bmask = maptileOne->format->Bmask;
    minimap->format->Amask = maptileOne->format->Amask;
    if( maptileOne->format->palette != NULL ) {
        SDL_SetColors(minimap, maptileOne->format->palette->colors, 0,
                      maptileOne->format->palette->ncolors);
    }
    int lineCounter = 0;
    for(unsigned int i = 0;  i < (unsigned int) width*height; i++, pos.x += pixsize,
            lineCounter++) {
        if(lineCounter == width) {
            pos.y += pixsize;
            pos.x = 0;
            lineCounter = 0;
        }
        cminitile = ip.minimapScale(getMapTile(i), pixsize);
        SDL_BlitSurface(cminitile, NULL, minimap, &pos);
        SDL_FreeSurface(cminitile);
    }
    /* Now fill in clipping details for renderer and UI.
     * To make things easier, ensure that the geometry is divisable by the
     * specified width and height.
     */
    tx = min(miniclip.sidew, (unsigned short)minimap->w);
    ty = min(tx, (unsigned short)minimap->h);
    // w == width in pixels of the minimap
    miniclip.w = pixsize*(int)floor((double)tx/(double)pixsize);
    miniclip.h = pixsize*(int)floor((double)ty/(double)pixsize);
    // x == offset in pixels from the top-left hand corner of the sidebar under
    // the tab.
    miniclip.x = abs(miniclip.sidew-miniclip.w)/2;
    miniclip.y = abs(miniclip.sidew-miniclip.h)/2;
    // Tilew == number of tiles visible in minimap horizontally
    miniclip.tilew = miniclip.w/pixsize;
    miniclip.tileh = miniclip.h/pixsize;
    // pixsize == number of pixels wide and high a minimap tile is
    miniclip.pixsize = pixsize;
    return minimap;
}

void CnCMap::storeLocation(unsigned char loc)
{
    if (loc >= NUMMARKS) {
        return;
    }
    scrollbookmarks[loc].x = scrollpos.curx;
    scrollbookmarks[loc].y = scrollpos.cury;
    scrollbookmarks[loc].xtile = scrollpos.curxtileoffs;
    scrollbookmarks[loc].ytile = scrollpos.curytileoffs;
}

void CnCMap::restoreLocation(unsigned char loc)
{
    if (loc >= NUMMARKS) {
        return;
    }
    scrollpos.curx = scrollbookmarks[loc].x;
    scrollpos.cury = scrollbookmarks[loc].y;
    scrollpos.curxtileoffs = scrollbookmarks[loc].xtile;
    scrollpos.curytileoffs = scrollbookmarks[loc].ytile;

    if( scrollpos.curxtileoffs >= scrollpos.tilewidth ) {
        scrollpos.curxtileoffs = scrollpos.tilewidth-1;
    }
    if( scrollpos.curytileoffs >= scrollpos.tilewidth ) {
        scrollpos.curytileoffs = scrollpos.tilewidth-1;
    }
    if( scrollpos.curx > scrollpos.maxx ) {
        scrollpos.curx = scrollpos.maxx;
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    } else if( scrollpos.curx == scrollpos.maxx &&
               scrollpos.curxtileoffs > scrollpos.maxxtileoffs ) {
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    }
    if( scrollpos.cury > scrollpos.maxy ) {
        scrollpos.cury = scrollpos.maxy;
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    } else if( scrollpos.cury == scrollpos.maxy &&
               scrollpos.curytileoffs > scrollpos.maxytileoffs ) {
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    }

    setValidScroll();
}

unsigned int CnCMap::translateToPos(unsigned short x, unsigned short y) const
{
    return y*width+x;
}

void CnCMap::translateFromPos(unsigned int pos, unsigned short *x, unsigned short *y) const
{
    *y = pos/width;
    *x = pos-((*y)*width);
}
