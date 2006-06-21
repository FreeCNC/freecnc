#include <stdexcept>
#include <cmath>

#include "../game/game_public.h"
#include "../ui/ui_public.h"
#include "graphicsengine.h"

using pc::imgcache;

/** Constructor, inits the sdl graphics and prepares to render maps
 * and draw vqaframes.
 * @param commandline arguments regarding video.
 */
GraphicsEngine::GraphicsEngine()
{
    ConfigType config;
    char* tmp;
    config = getConfig();
    width = config.width;
    height = config.height;

    /* Set the caption, not nessesary but nice =) */
    tmp = new char[128];
    switch (config.gamemode) {
    case 0:
        sprintf(tmp,"FreeCNC - %s",config.mapname.c_str());
        break;
    case 1:
        sprintf(tmp,"FreeCNC - skirmish on %s with %i bots",config.mapname.c_str(), config.totalplayers-1);
        break;
    case 2:
        sprintf(tmp,"FreeCNC - multiplayer on %s as %s (%i/%i)",config.mapname.c_str(),
                config.nick.c_str(),config.playernum,config.totalplayers);
        break;
    default:
        sprintf(tmp,"THIS IS A BUG. (unknown mode: %i)",config.gamemode);
        break;
    };
    SDL_WM_SetCaption(tmp, NULL);
    delete[] tmp;
    SDL_WM_GrabInput(config.grabmode);

    // Init the screen
    icon = SDL_LoadBMP("data/gfx/icon.bmp");
    if (icon != 0)
        SDL_WM_SetIcon(icon, NULL);
    screen = SDL_SetVideoMode(width, height, config.bpp, config.videoflags);

    if (screen == NULL) {
        logger->error("Unable to set %dx%d video: %s\n",
                      width, height, SDL_GetError());
        throw VideoError();
    }

    imgcache = new ImageCache();

    try {
        pc::msg = new MessagePool();
        pc::msg->setWidth(width);
    } catch(int) {
        logger->error("Unable to create message pool (missing font?)\n");
        throw VideoError();
    }
    logger->renderGameMsg(true);
}



/** Destructor, free the memory used by the graphicsengine. */
GraphicsEngine::~GraphicsEngine()
{
    delete imgcache;
    logger->renderGameMsg(false);
    delete pc::msg;
    pc::msg = NULL;
    SDL_FreeSurface(icon);
}

/** Setup the vars for the current mission.
 * @param the current map.
 * @param the current sidebar.
 * @param the current cursor.
 */
void GraphicsEngine::setupCurrentGame()
{
    /* assume the width of tile 0 is correct */
    tilewidth = p::ccmap->getMapTile(0)->w;

    // assume sidebar is visible for initial maparea (will be changed at
    // frame 1 if it isn't anyway)
    maparea.h = height-pc::sidebar->getTabLocation()->h;
    maparea.y = pc::sidebar->getTabLocation()->h;
    maparea.w = width-pc::sidebar->getTabLocation()->w;
    maparea.x = 0;
    if (maparea.w > p::ccmap->getWidth()*tilewidth) {
        maparea.w = p::ccmap->getWidth()*tilewidth;
    }
    if (maparea.h > p::ccmap->getHeight()*tilewidth) {
        maparea.h = p::ccmap->getHeight()*tilewidth;
    }
    p::ccmap->setMaxScroll(maparea.w/tilewidth, maparea.h/tilewidth,
                      maparea.w%tilewidth, maparea.h%tilewidth, tilewidth);
    if (getConfig().bpp == 8) {
        // This doesn't work, but the cursors look nicer so leaving it in to
        // remind me to look at cursor rendering again.
        SDL_SetColors(screen, SHPBase::getPalette(0), 0, 256);
    }
    /* this is a new game so clear the scren */
    clearScreen();
    p::ccmap->prepMiniClip(pc::sidebar->getTabLocation()->w,pc::sidebar->getTabLocation()->h);
    minizoom.normal = (tilewidth*pc::sidebar->getTabLocation()->w)/max(maparea.w, maparea.h);
    minizoom.max    = max(1,tilewidth*pc::sidebar->getTabLocation()->w / max(
                p::ccmap->getWidth()*tilewidth, p::ccmap->getHeight()*tilewidth));
    mz = &minizoom.normal;
    playercolours.resize(SHPBase::numPalettes());
    for (unsigned char i = 0; i < playercolours.size() ; ++i) {
        // Magic Number 179: See comment at start of
        // shpimage.cpp
        playercolours[i] = SHPBase::getColour(screen->format, i, 179);
    }
}

void GraphicsEngine::clipToMaparea(SDL_Rect *dest)
{
    if (dest->x < maparea.x) {
        dest->w -= maparea.x-dest->x;
        dest->x = maparea.x;
    }
    if (dest->y < maparea.y) {
        dest->h = maparea.y - dest->y;
        dest->y = maparea.y;
    }
}

void GraphicsEngine::clipToMaparea(SDL_Rect *src, SDL_Rect *dest)
{
    if (dest->x < maparea.x) {
        src->x = maparea.x - dest->x;
        src->w -= src->x;
        dest->x = maparea.x;
        dest->w = src->w;
    }
    if (dest->y < maparea.y) {
        src->y = maparea.y - dest->y;
        src->h -= src->y;
        dest->y = maparea.y;
        dest->h = src->h;
    }
}

/** Render a scene complete with map, sidebar and cursor. */
void GraphicsEngine::renderScene()
{
    //   int mx, my;
    int i;
    short xpos, ypos;

    SDL_Surface *curimg, *bgimage;
    SDL_Rect dest, src, udest, oldudest;

    unsigned int tiberium, smudge, overlay, terrain;
    unsigned char numshps;
    unsigned int *unitorstructshps;
    char *uxoffsets, *uyoffsets;
    short txoff, tyoff;
    double ratio;
    Unit* tmp_un;

    std::vector<unsigned short> l2overlays;

    Player* lplayer = p::ppool->getLPlayer();
    std::vector<bool>& mapvis = lplayer->getMapVis();

    static unsigned int whitepix = SDL_MapRGB(screen->format,0xff, 0xff, 0xff);
    static unsigned int greenpix = SDL_MapRGB(screen->format,0, 0xff, 0);
    static unsigned int yellowpix = SDL_MapRGB(screen->format,0xff, 0xff, 0);
    static unsigned int redpix = SDL_MapRGB(screen->format,0xff, 0, 0);
    static unsigned int blackpix = SDL_MapRGB(screen->format,0, 0, 0);

    unsigned short mapWidth, mapHeight, selection;
    unsigned int curpos, curdpos;

    char xmax, ymax;
    static SDL_Rect oldmouse = {0, 0, 0, 0};
    /* remove the old mousecursor */
    SDL_FillRect(screen, &oldmouse, blackpix);

    drawSidebar();

    /* first thing we want to do is scroll the map */
    if (p::ccmap->toScroll())
        p::ccmap->doscroll();

    mapWidth = (maparea.w+p::ccmap->getXTileScroll()+tilewidth-1)/tilewidth;
    mapWidth = min(mapWidth, p::ccmap->getWidth());
    mapHeight = (maparea.h+p::ccmap->getYTileScroll()+tilewidth-1)/tilewidth;
    mapHeight = min(mapHeight, p::ccmap->getHeight());

    /*draw minimap*/
    if (Input::isMinimapEnabled()) {
        const unsigned char minizoom = *mz;
        const unsigned int mapwidth = p::ccmap->getWidth();
        const unsigned int mapheight = p::ccmap->getHeight();
        const MiniMapClipping& clip = p::ccmap->getMiniMapClipping();
        unsigned char minx;
        unsigned int cury;
        bool blocked;
        // Need the exact dimensions in tiles
        // @TODO Positioning needs tweaking
        SDL_Surface *minimap = p::ccmap->getMiniMap(minizoom);
        // Draw black under minimap if haven't previously drawn (or was drawn,
        // then disabled, then reenabled).
        dest.w = clip.w;
        dest.h = clip.h;
        dest.x = maparea.x+maparea.w+clip.x;
        dest.y = maparea.y+clip.y;

        src.x = p::ccmap->getXScroll()*minizoom;
        src.y = p::ccmap->getYScroll()*minizoom;
        src.w = dest.w;
        src.h = dest.h;
        if (src.x + src.w >= minimap->w) {
            src.x = minimap->w - src.w;
        }
        if (src.y + src.h >= minimap->h) {
            src.y = minimap->h - src.h;
        }
        SDL_BlitSurface(minimap, &src, screen, &dest);
        // Draw black over not visible parts
        curpos = p::ccmap->getScrollPos();
        minx = min(curpos%mapwidth, mapwidth - clip.tilew);
        cury = min(curpos/mapwidth, mapheight - clip.tileh);
        curpos = minx+cury*mapwidth;
        for (ypos = 0; ypos < clip.tileh ; ++ypos) {
            for (xpos = 0 ; xpos < clip.tilew ; ++xpos) {
                if (mapvis[curpos]) {
                    float width, height;
                    unsigned char igroup, owner, pcol;
                    unsigned int cellpos;
                    // Rather than make the graphics engine depend on the
                    // UnitOrStructureType, just pull what we need from the
                    // USPool.
                    if (p::uspool->getUnitOrStructureLimAt(curpos, &width,
                            &height, &cellpos, &igroup, &owner, &pcol, &blocked)) {
                        /// @TODO drawing infanty groups as smaller pixels
                        if (blocked) {
                            dest.x = maparea.x+maparea.w+clip.x+xpos*minizoom;
                            dest.y = maparea.y+clip.y+ypos*minizoom;
                            dest.w = (unsigned short)ceil(width*minizoom);
                            dest.h = (unsigned short)ceil(height*minizoom);
                            SDL_FillRect(screen, &dest, playercolours[pcol]);
                        }
                    }
                } else {
                    // Draw a black square here
                    dest.x = maparea.x+maparea.w+clip.x+xpos*minizoom;
                    dest.y = maparea.y+clip.y+ypos*minizoom;
                    dest.w = minizoom;
                    dest.h = dest.w;
                    SDL_FillRect(screen, &dest, blackpix);
                }
                ++curpos;
            }
            ++cury;
            curpos = minx+cury*mapwidth;
        }
    }
    SDL_SetClipRect( screen, &maparea);

    dest.y = maparea.y-p::ccmap->getYTileScroll();

    xmax = min(p::ccmap->getWidth()-(p::ccmap->getXScroll()+mapWidth), 4);
    ymax = min(p::ccmap->getHeight()-(p::ccmap->getYScroll()+mapHeight), 4);

    curpos = p::ccmap->getScrollPos();
    for( ypos = 0; ypos < mapHeight+ymax+2; ypos++) {
        dest.x = maparea.x-p::ccmap->getXTileScroll();
        for( xpos = 0; xpos < mapWidth+xmax; xpos++) {
            dest.w = tilewidth;
            dest.h = tilewidth;

            if (xpos < mapWidth && ypos < mapHeight) {
                udest.x = dest.x;
                udest.y = dest.y;
                udest.w = tilewidth;
                udest.h = tilewidth;
                src.x = 0;
                src.y = 0;
                src.w = tilewidth;
                src.h = tilewidth;
                clipToMaparea(&src, &udest);

                bgimage = p::ccmap->getMapTile(curpos);
                if (bgimage != NULL) {
                    SDL_BlitSurface(bgimage, &src, screen, &udest);
                }

                smudge = p::ccmap->getSmudge(curpos);
                if (smudge != 0) {
                    ImageCacheEntry& images = imgcache->getImage(smudge);
                    SDL_BlitSurface(images.image, &src, screen, &udest);
                    SDL_BlitSurface(images.shadow, &src, screen, &udest);
                }

                tiberium = p::ccmap->getResourceFrame(curpos);
                if (tiberium != 0) {
                    ImageCacheEntry& images = imgcache->getImage(tiberium);
                    SDL_BlitSurface(images.image, &src, screen, &udest);
                }

                overlay = p::ccmap->getOverlay(curpos);
                if (overlay != 0) {
                    ImageCacheEntry& images = imgcache->getImage(overlay);
                    SDL_BlitSurface(images.image, &src, screen, &udest);
                    SDL_BlitSurface(images.shadow, &src, screen, &udest);
                }

                if (p::uspool->hasL2overlay(curpos)) {
                    l2overlays.push_back(curpos);
                }
            }
            if (ypos > 1) {
                dest.y -= (tilewidth<<1);
                curdpos = curpos-(p::ccmap->getWidth()<<1);
                terrain = p::ccmap->getTerrain(curdpos, &txoff, &tyoff);
                if (terrain != 0) {
                    ImageCacheEntry& images = imgcache->getImage(terrain);
                    src.x = 0;
                    src.y = 0;
                    src.w = images.image->w;
                    src.h = images.image->h;
                    udest.x = dest.x+txoff;
                    udest.y = dest.y+tyoff;
                    udest.w = images.image->w;
                    udest.h = images.image->h;
                    clipToMaparea(&src, &udest);

                    SDL_BlitSurface(images.image, &src, screen, &udest);
                    SDL_BlitSurface(images.shadow, &src, screen, &udest);
                }

                numshps = p::uspool->getUnitOrStructureNum(curdpos, &unitorstructshps,
                          &uxoffsets, &uyoffsets);
                oldudest.x = oldudest.y = oldudest.w = oldudest.h = 0;
                if (numshps > 0) {
                    for( i  = 0; i < numshps; i++) {
                        ImageCacheEntry& images = imgcache->getImage(unitorstructshps[i]);
                        src.x = 0;
                        src.y = 0;
                        src.w = images.image->w;
                        src.h = images.image->h;
                        udest.w = images.image->w;
                        udest.h = images.image->h;
                        udest.x = dest.x+uxoffsets[i];
                        udest.y = dest.y+uyoffsets[i];
                        clipToMaparea(&src, &udest);

                        SDL_BlitSurface(images.image, &src, screen, &udest);
                        SDL_BlitSurface(images.shadow, &src, screen, &udest);
                    }
                    delete[] unitorstructshps;
                    delete[] uxoffsets;
                    delete[] uyoffsets;
                }
                // draw health bars
                selection = p::uspool->getSelected(curdpos);
                if ((selection&0xff) != 0) {
                    udest.h = 5;
                    if (selection>>8 == 0xff) {
                        // safe to assume Unit
                        if (selection &1) {
                            tmp_un  = (Unit*)p::uspool->getUnitOrStructureAt(curdpos,0);
                            ratio   = tmp_un->getRatio();
                            udest.x = dest.x + 6 + tmp_un->getXoffset();
                            udest.y = dest.y + 5 + tmp_un->getYoffset();
                            clipToMaparea(&udest);
                            udest.w = 12;
                            udest.h = 5;
                            SDL_FillRect(screen, &udest, blackpix);
                            udest.w = (unsigned short)(10.0 * ratio);
                            udest.h -= 2;
                            ++udest.x;
                            ++udest.y;
                            SDL_FillRect(screen, &udest, ((ratio<=0.5)?(ratio<=0.25?redpix:yellowpix):greenpix));
                        }
                        if (selection &2) {
                            tmp_un  = (Unit*)p::uspool->getUnitOrStructureAt(curdpos,1);
                            ratio   = tmp_un->getRatio();
                            udest.x = dest.x + tmp_un->getXoffset();
                            udest.y = dest.y + tmp_un->getYoffset();
                            clipToMaparea(&udest);
                            udest.w = 12;
                            udest.h = 5;
                            SDL_FillRect(screen, &udest, blackpix);
                            udest.w = (unsigned short)(10.0 * ratio);
                            udest.h -= 2;
                            ++udest.x;
                            ++udest.y;
                            SDL_FillRect(screen, &udest, ((ratio<=0.5)?(ratio<=0.25?redpix:yellowpix):greenpix));
                        }
                        if (selection &4) {
                            tmp_un = (Unit*)p::uspool->getUnitOrStructureAt(curdpos,2);
                            ratio   = tmp_un->getRatio();
                            udest.x = dest.x + 12 + tmp_un->getXoffset();
                            udest.y = dest.y + tmp_un->getYoffset();
                            clipToMaparea(&udest);
                            udest.w = 12;
                            udest.h = 5;
                            SDL_FillRect(screen, &udest, blackpix);
                            udest.w = (unsigned short)(10.0 * ratio);
                            udest.h -= 2;
                            ++udest.x;
                            ++udest.y;
                            SDL_FillRect(screen, &udest, ((ratio<=0.5)?(ratio<=0.25?redpix:yellowpix):greenpix));
                        }
                        if (selection &8) {
                            tmp_un = (Unit*)p::uspool->getUnitOrStructureAt(curdpos,3);
                            ratio   = tmp_un->getRatio();
                            udest.x = dest.x + tmp_un->getXoffset();
                            udest.y = dest.y + 10 + tmp_un->getYoffset();
                            clipToMaparea(&udest);
                            udest.w = 12;
                            udest.h = 5;
                            SDL_FillRect(screen, &udest, blackpix);
                            udest.w = (unsigned short)(10.0 * ratio);
                            udest.h -= 2;
                            ++udest.x;
                            ++udest.y;
                            SDL_FillRect(screen, &udest, ((ratio<=0.5)?(ratio<=0.25?redpix:yellowpix):greenpix));
                        }
                        if (selection &16) {
                            tmp_un = (Unit*)p::uspool->getUnitOrStructureAt(curdpos,4);
                            ratio   = tmp_un->getRatio();
                            udest.x = dest.x + 12 + tmp_un->getXoffset();
                            udest.y = dest.y + 10 + tmp_un->getYoffset();
                            clipToMaparea(&udest);
                            udest.w = 12;
                            udest.h = 5;
                            SDL_FillRect(screen, &udest, blackpix);
                            udest.w = (unsigned short)(10.0 * ratio);
                            udest.h -= 2;
                            ++udest.x;
                            ++udest.y;
                            SDL_FillRect(screen, &udest, ((ratio<=0.5)?(ratio<=0.25?redpix:yellowpix):greenpix));
                        }

                    } else if (udest.w >= 2) {
                        ratio = p::uspool->getUnitOrStructureAt(curdpos,0)->getRatio();
                        SDL_FillRect(screen, &udest, blackpix);
                        udest.h -= 2;
                        ++udest.x;
                        ++udest.y;
                        udest.w = (unsigned short)((double)(udest.w-2) * ratio);
                        SDL_FillRect(screen, &udest, ((ratio<=0.5)?(ratio<=0.25?redpix:yellowpix):greenpix));
                    }

                }

                dest.y += (tilewidth<<1);
            } /* ypos > 1 */
            /* show the different tiles */
            /*
            udest.x = maparea.x;
            udest.y = maparea.y+ypos*24;
            udest.h = 1;
            udest.w = maparea.w;
            SDL_FillRect(screen, &udest, whitepix); 
            udest.x = maparea.x+xpos*24; 
            udest.y = maparea.y; 
            udest.h = maparea.h;
            udest.w = 1;
            SDL_FillRect(screen, &udest, whitepix);
            */
            dest.x += tilewidth;
            curpos++;
        }

        curpos += p::ccmap->getWidth()-mapWidth-xmax;
        dest.y += tilewidth;
    }
    /* draw the level two overlays */
    for( i = 0; (unsigned int)i < l2overlays.size(); i++) {
        curpos = l2overlays[i];
        dest.x = maparea.x-p::ccmap->getXTileScroll()+(curpos%p::ccmap->getWidth()-p::ccmap->getXScroll())*tilewidth;
        dest.y = maparea.y-p::ccmap->getYTileScroll()+(curpos/p::ccmap->getWidth()-p::ccmap->getYScroll())*tilewidth;
        numshps = p::uspool->getL2overlays(curpos, &unitorstructshps, &uxoffsets, &uyoffsets);
        for( curdpos = 0; curdpos < numshps; curdpos++) {
            ImageCacheEntry& images = imgcache->getImage(unitorstructshps[curdpos]);
            udest.x = dest.x + uxoffsets[curdpos];
            udest.y = dest.y + uyoffsets[curdpos];
            udest.w = images.image->w;
            udest.h = images.image->h;
            SDL_BlitSurface(images.image, NULL, screen, &udest);
        }
        delete[] unitorstructshps;
        delete[] uxoffsets;
        delete[] uyoffsets;
    }

    /* draw black on all non-visible squares */
    dest.w = tilewidth;
    dest.h = tilewidth;
    curpos = p::ccmap->getScrollPos();
    dest.y = maparea.y-p::ccmap->getYTileScroll();
    /** @todo: This uses hardcoded values which it shouldn't do. It should also cache the imagenums. If the imagenum is out of reach NULL will be returned, we should check for this*/
    int shadowoffs;
    for( ypos = 0; ypos < mapHeight; ypos++) {
        dest.x = maparea.x-p::ccmap->getXTileScroll();
        for( xpos = 0; xpos < mapWidth; xpos++) {

            udest.x = dest.x;
            udest.y = dest.y;
            udest.w = tilewidth;
            udest.h = tilewidth;
            if (mapvis[curpos]) {
                src.x = 0;
                src.y = 0;
                src.w = tilewidth;
                src.h = tilewidth;
                clipToMaparea(&src, &udest);
                i = 0;
                if (curpos >= p::ccmap->getWidth() && !mapvis[curpos-p::ccmap->getWidth()]) {
                    i |= 1;
                }
                if (curpos%p::ccmap->getWidth() < (unsigned short)(p::ccmap->getWidth()-1) && !mapvis[curpos+1]) {
                    i |= 2;
                }
                if (curpos < (unsigned int)p::ccmap->getWidth()*(p::ccmap->getHeight()-1) && !mapvis[curpos+p::ccmap->getWidth()]) {
                    i |= 4;
                }
                if (curpos%p::ccmap->getWidth() > 0 && !mapvis[curpos-1]) {
                    i |= 8;
                }
                //Allows a random title to be drawn
                shadowoffs = 12*((curpos + curpos/p::ccmap->getWidth())%4);
                //shadowoffs = 12*((curpos)%4); // Simplier, but maybe wrong?
                if (i != 0) {
                    if (i&1) { // Top
                        SDL_BlitSurface(p::ccmap->getShadowTile(3+shadowoffs), &src, screen, &udest);
                    }
                    if (i&2) { // Right
                        SDL_BlitSurface(p::ccmap->getShadowTile(5+shadowoffs), &src, screen, &udest);
                    }
                    if (i&4) { // Bottom
                        SDL_BlitSurface(p::ccmap->getShadowTile(0+shadowoffs), &src, screen, &udest);
                    }
                    if (i&8) { // Left
                        SDL_BlitSurface(p::ccmap->getShadowTile(1+shadowoffs), &src, screen, &udest);
                    }
                    if ((i&3) == 3) { // Top Right
                        SDL_BlitSurface(p::ccmap->getShadowTile(10+shadowoffs), &src, screen, &udest);
                    }
                    if ((i&6) == 6) { // Bottom Right
                        SDL_BlitSurface(p::ccmap->getShadowTile(11+shadowoffs), &src, screen, &udest);
                    }
                    if ((i&12) == 12) { // Bottom Left
                        SDL_BlitSurface(p::ccmap->getShadowTile(8+shadowoffs), &src, screen, &udest);
                    }
                    if ((i&9) == 9) { // Top Left
                        SDL_BlitSurface(p::ccmap->getShadowTile(9+shadowoffs), &src, screen, &udest);
                    }
                } else {
                    if (curpos >= p::ccmap->getWidth() && curpos%p::ccmap->getWidth() < (unsigned short)(p::ccmap->getWidth()-1) && !mapvis[curpos-p::ccmap->getWidth()+1]) {
                        i |= 1;
                    }
                    if (curpos < (unsigned int)p::ccmap->getWidth()*(p::ccmap->getHeight()-1) && curpos%p::ccmap->getWidth() < (unsigned short)(p::ccmap->getWidth()-1) && !mapvis[curpos+p::ccmap->getWidth()+1]) {
                        i |= 2;
                    }
                    if (curpos < (unsigned int)p::ccmap->getWidth()*(p::ccmap->getHeight()-1) && curpos%p::ccmap->getWidth() > 0 && !mapvis[curpos+p::ccmap->getWidth()-1]) {
                        i |= 4;
                    }
                    if (curpos >= p::ccmap->getWidth() && curpos%p::ccmap->getWidth() > 0 && !mapvis[curpos-p::ccmap->getWidth()-1]) {
                        i |= 8;
                    }

                    switch(i) {
                    case 1: 
                        SDL_BlitSurface(p::ccmap->getShadowTile(7+shadowoffs), &src, screen, &udest);
                        break;
                    case 2: // Bottom Right
                        SDL_BlitSurface(p::ccmap->getShadowTile(6+shadowoffs), &src, screen, &udest);
                        break;
                    case 4:
                        SDL_BlitSurface(p::ccmap->getShadowTile(2+shadowoffs), &src, screen, &udest);
                        break;
                    case 8: // Top Left
                        SDL_BlitSurface(p::ccmap->getShadowTile(4+shadowoffs), &src, screen, &udest);
                        break;
                    default:
                        break;
                    }
                }
            } else {
                // draw a black square here
                SDL_FillRect(screen, &udest, blackpix);
            }

            dest.x += tilewidth;
            curpos++;
        }
        curpos += p::ccmap->getWidth()-mapWidth;
        dest.y += tilewidth;
    }

    /* draw the selectionbox */
    if (Input::isDrawing()) {
        dest.x = min(Input::getMarkRect().x, (short)Input::getMarkRect().w);
        dest.y = min(Input::getMarkRect().y, (short)Input::getMarkRect().h);
        dest.w = abs(Input::getMarkRect().x - Input::getMarkRect().w);
        dest.h = 1;
        SDL_FillRect(screen, &dest, whitepix);
        dest.y += abs(Input::getMarkRect().y - Input::getMarkRect().h);
        SDL_FillRect(screen, &dest, whitepix);
        dest.y -= abs(Input::getMarkRect().y - Input::getMarkRect().h);
        dest.h = abs(Input::getMarkRect().y - Input::getMarkRect().h);
        dest.w = 1;
        SDL_FillRect(screen, &dest, whitepix);
        dest.x += abs(Input::getMarkRect().x - Input::getMarkRect().w);
        SDL_FillRect(screen, &dest, whitepix);
    }

    dest.x = dest.y = 0;
    dest.w = width;
    dest.h = height;
    SDL_SetClipRect( screen, &dest);

    //   SDL_GetMouseState(&mx, &my);

    //   dest.x = mx;
    //   dest.y = my;
    // Draw messages
    curimg = pc::msg->getMessages();
    if (curimg != NULL) {
        dest.x = maparea.x;
        dest.y = maparea.y;
        dest.w = curimg->w;
        dest.h = curimg->h;

        SDL_BlitSurface(curimg, NULL, screen, &dest);
    }

    // Draw the mouse
    dest.x = pc::cursor->getX();
    dest.y = pc::cursor->getY();
    if (dest.x < 0)
        dest.x = 0;
    if (dest.y < 0)
        dest.y = 0;

    curimg = pc::cursor->getCursor();
    dest.w = curimg->w;
    dest.h = curimg->h;

    //oldmouse = dest;
    SDL_BlitSurface(curimg, NULL, screen, &dest);

    // just here to test drawLine
    //drawLine(-160, -120, dest.x, dest.y, 5, blackpix);

    SDL_Flip(screen);
#ifdef _WIN32
    SDL_FillRect(screen, &oldmouse, blackpix);
#endif

    oldmouse = dest;
}



/** Draw the sidebar and tabs. */
void GraphicsEngine::drawSidebar()
{
    SDL_Rect dest;
    SDL_Rect *tabpos;

    static unsigned int firstframe = SDL_GetTicks();
    static unsigned int frames = 0;
    static bool clearBack = false;
    //    static unsigned int blackpix = SDL_MapRGB(screen->format, 0, 0, 0);
    unsigned int tick;
    unsigned short framerate;
    char mtext[128];

    Player* lplayer = p::ppool->getLPlayer();
    /* clear parts of the screen not overwriten by the sidebar or map
     remove this code */
    /*    dest.x = 0;
        dest.y = 0;
        dest.w = width;
        dest.h = maparea.y;
        SDL_FillRect(screen, &dest, blackpix);
        dest.y = maparea.y+maparea.h;
        dest.h = height-dest.y;
        SDL_FillRect(screen, &dest, blackpix);
        dest.y = maparea.y;
        dest.w = maparea.x;
        dest.h = maparea.h;
        SDL_FillRect(screen, &dest, blackpix);
        dest.x = maparea.x+maparea.w;
        dest.w = width - dest.x
        SDL_FillRect(screen, &dest, blackpix);
    */
    tabpos = pc::sidebar->getTabLocation();

    if (clearBack) {
        clearBuffer();
        clearBack = false;
    }

    if (pc::sidebar->getVisible()) {
        if (pc::sidebar->getVisChanged()) {
            clearBuffer();
            clearBack = true;

            /*maparea.h = ((height-tabpos->h)/tilewidth)*tilewidth;
            maparea.y = ((height-tabpos->h-maparea.h)>>1)+tabpos->h;
            maparea.w = ((width-tabpos->w)/tilewidth)*tilewidth;
            maparea.x = (width-tabpos->w-maparea.w)>>1;
            p::ccmap->setMaxScroll(maparea.w/tilewidth, maparea.h/tilewidth);*/
            maparea.h = height-tabpos->h;
            maparea.y = tabpos->h;
            maparea.w = width-tabpos->w;
            maparea.x = 0;
            if (maparea.w > p::ccmap->getWidth()*tilewidth) {
                maparea.w = p::ccmap->getWidth()*tilewidth;
                maparea.x = (width-tabpos->w-maparea.w)>>1;
            }
            if (maparea.h > p::ccmap->getHeight()*tilewidth) {
                maparea.h = p::ccmap->getHeight()*tilewidth;
                maparea.y = tabpos->h+((height-tabpos->h-maparea.h)>>1);
            }
            p::ccmap->setMaxScroll(maparea.w/tilewidth, maparea.h/tilewidth,
                              maparea.w%tilewidth, maparea.h%tilewidth, tilewidth);
        }
        dest.x = width - tabpos->w;
        dest.w = tabpos->w;
        dest.y = tabpos->h;
        dest.h = height-tabpos->h;

        //      SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format, 0xa0, 0xa0, 0xa0));
        SDL_BlitSurface(pc::sidebar->getSidebarImage(dest), NULL, screen, &dest);
    } else if (pc::sidebar->getVisChanged()) {
        clearBuffer();
        clearBack = true;

        /*maparea.h = ((height-tabpos->h)/tilewidth)*tilewidth;
        maparea.y = ((height-tabpos->h-maparea.h)>>1)+tabpos->h;
        maparea.w = (width/tilewidth)*tilewidth;
        maparea.x = (width-maparea.w)>>1;
        p::ccmap->setMaxScroll(maparea.w/tilewidth, maparea.h/tilewidth);*/
        maparea.h = height-tabpos->h;
        maparea.y = tabpos->h;
        maparea.w = width;
        maparea.x = 0;
        if (maparea.w > p::ccmap->getWidth()*tilewidth) {
            maparea.w = p::ccmap->getWidth()*tilewidth;
            maparea.x = (width-maparea.w)>>1;
        }
        if (maparea.h > p::ccmap->getHeight()*tilewidth) {
            maparea.h = p::ccmap->getHeight()*tilewidth;
            maparea.y = tabpos->h+((height-tabpos->h-maparea.h)>>1);
        }
        p::ccmap->setMaxScroll(maparea.w/tilewidth, maparea.h/tilewidth,
                          maparea.w%tilewidth, maparea.h%tilewidth, tilewidth);
    }

    tabpos->y = 0;//maparea.y-tabpos->h;
    tabpos->x = 0;

    SDL_BlitSurface(pc::sidebar->getTabImage(), NULL, screen, tabpos);

    dest.y = tabpos->y+tabpos->h-pc::sidebar->getFont()->getHeight();
    dest.x = ((tabpos->w-pc::sidebar->getFont()->calcTextWidth("Options"))>>1)+tabpos->x;
    pc::sidebar->getFont()->drawText("Options", screen, dest.x, dest.y);

    tabpos->x = width - tabpos->w;

    SDL_BlitSurface( pc::sidebar->getTabImage(), NULL, screen, tabpos);
    sprintf(mtext, "%d", lplayer->getMoney());
    dest.x = ((tabpos->w-pc::sidebar->getFont()->calcTextWidth(mtext))>>1)+tabpos->x;
    pc::sidebar->getFont()->drawText(mtext, screen, dest.x, dest.y);


    tick = SDL_GetTicks();
    frames++;
    framerate = (frames*1000)/(tick - firstframe+1);
    sprintf(mtext, "%d fps", framerate);
    SDL_Rect clearCoor;
    clearCoor.w = width-2*tabpos->w;/*100;*/
    clearCoor.h = tabpos->h;/*20;*/
    clearCoor.x = tabpos->w;/*width >>1;*/
    clearCoor.y = dest.y;
    SDL_FillRect(screen, &clearCoor, SDL_MapRGB(screen->format, 0, 0, 0));
    pc::sidebar->getFont()->drawText(mtext, screen, tabpos->w/*width>>1*/, dest.y);
    if (frames == 1000) {
        frames = 1;
        firstframe = tick;
    }
}



/** draw a vqafarame to the screen.
 * @param the vqaframe.
 * @param where to draw the frame.
 */
void GraphicsEngine::drawVQAFrame(SDL_Surface *frame)
{
    SDL_Rect dest;
    dest.w = frame->w;
    dest.h = frame->h;
    dest.x = (screen->w-frame->w)>>1;
    dest.y = (screen->h-frame->h)>>1;
    SDL_BlitSurface(frame, NULL, screen, &dest);
    SDL_Flip(screen);
}



/** Clear the frontBuffer, i.e. paint it black. */
void GraphicsEngine::clearBuffer()
{
    static unsigned int blackpix = SDL_MapRGB(screen->format, 0, 0, 0);
    SDL_Rect dest;

    dest.x = 0;
    dest.y = 0;
    dest.w = width;
    dest.h = height;

    SDL_FillRect(screen, &dest, blackpix);

}

/** Clear the screen, i.e. paint it black. */
void GraphicsEngine::clearScreen()
{
    static unsigned int blackpix = SDL_MapRGB(screen->format, 0, 0, 0);
    SDL_Rect dest;

    dest.x = 0;
    dest.y = 0;
    dest.w = width;
    dest.h = height;

    SDL_FillRect(screen, &dest, blackpix);

    /* directx (and possibly other platforms needs to clear both
    front and back buffer */

    SDL_Flip(screen);
    SDL_FillRect(screen, &dest, blackpix);

}

void GraphicsEngine::renderLoading(const std::string& buff, SDL_Surface* logo)
{
    static unsigned int blackpix = SDL_MapRGB(screen->format, 0, 0, 0);
    SDL_Rect dest;
    SDL_Surface *curimg;
    int i;
    static int numdots = 0;

    dest.x = 0;
    dest.y = 0;
    dest.w = width;
    dest.h = height;

    SDL_FillRect(screen, &dest, blackpix);

    if (logo != 0) {
        dest.x = (width-logo->w)/2;
        dest.y = (height-logo->h)/2;
        SDL_BlitSurface(logo,NULL,screen,&dest);
    }
    dest.x = dest.y = 0;

    std::string msg(buff);
    for (i = 0; i < numdots; ++i) {
        msg += ".";
    }
    numdots = (numdots+1)%4;

    pc::msg->postMessage(msg);

    curimg = pc::msg->getMessages();
    if (curimg != NULL) {
        // This used to center the text, but since moving to a fixed width
        // message pool, this isn't quite centerred
        dest.x = (width-curimg->w)/2;
        dest.y = (height-curimg->h)/2;
        dest.w = curimg->w;
        dest.h = curimg->h;
        SDL_BlitSurface(curimg, NULL, screen, &dest);
    }
    pc::msg->clear();

    SDL_Flip(screen);

}

void GraphicsEngine::drawLine(short startx, short starty,
                              short stopx, short stopy, unsigned short width, unsigned int colour)
{
    float xmod, ymod, length, xpos, ypos;
    int i, len;
    SDL_Rect dest;
    xmod = static_cast<float>(stopx-startx);
    ymod = static_cast<float>(stopy-starty);

    length = sqrt(xmod*xmod+ymod*ymod);
    xmod /= length;
    ymod /= length;

    len = static_cast<int>(length+0.5f);
    xpos = static_cast<float>(startx-(width>>1));
    ypos = static_cast<float>(starty-(width>>1));
    for(i = 0; i < len; i++) {
        dest.x = (short)xpos;
        dest.y = (short)ypos;
        dest.w = width;
        dest.h = width;
        SDL_FillRect(screen, &dest, colour);
        xpos += xmod;
        ypos += ymod;
    }
}

