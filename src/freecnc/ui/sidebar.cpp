#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include <boost/mem_fn.hpp>

#include "../game/game_public.h"
#include "../sound/sound_public.h"
#include "../vfs/vfs_public.h"
#include "input.h"
#include "sidebar.h"

/// @TODO Move this into config file(s)
static const char* radarnames[] =
{
//   Gold Edition radars: NOD, GDI and Dinosaur
    "hradar.nod", "hradar.gdi", "hradar.jp",
//   Dos Edition radars
    "radar.nod",  "radar.gdi",  "radar.jp",
//   Red Alert radars (We only use large sidebar for RA)
    "ussrradr.shp", "natoradr.shp", "natoradr.shp"
};

class ImageCache;

using std::string;
using std::runtime_error;
using std::vector;
using std::replace;

/**
 * @param pl local player object
 * @param height height of screen in pixels
 * @param theatre terrain type of the theatre
 *      TD: desert, temperate and winter
 *      RA: temperate and snow
 */
Sidebar::Sidebar(Player *pl, unsigned short height, const char *theatre)
    : radarlogo(0), tab(0), sbar(0), gamefnt(new Font("scorefnt.fnt")), visible(true),
    vischanged(true), theatre(theatre), buttondown(0), bd(false),
    radaranimating(false), unitoff(0), structoff(0), player(pl), scaleq(-1)
{
    const char* tmpname;
    unsigned char side;
    SDL_Surface *tmp;

    // If we can't load these files, there's no point in proceeding, thus we let
    // the default handler for runtime_error in main() catch.
    if (game.config.gametype == GAME_TD) {
        tmpname = VFS_getFirstExisting(2,"htabs.shp","tabs.shp");
        if (tmpname == NULL) {
            throw runtime_error("Unable to find the tab images! (Missing updatec.mix?)");
        } else if (strcasecmp(tmpname,"htabs.shp")==0) {
            isoriginaltype = false;
        } else {
            isoriginaltype = true;
        }
        try {
            tab = pc::imgcache->loadImage(tmpname, scaleq);
        } catch (ImageNotFound&) {
            throw runtime_error("Unable to load the tab images!");
        }
    } else {
        isoriginaltype = false;
        try {
            tab = pc::imgcache->loadImage("tabs.shp", scaleq);
        } catch (ImageNotFound&) {
            throw runtime_error("Unable to load the tab images!");
        }
    }

    tmp = pc::imgcache->getImage(tab).image;

    tablocation.x = 0;
    tablocation.y = 0;
    tablocation.w = tmp->w;
    tablocation.h = tmp->h;

    spalnum = pl->getStructpalNum();
    if (!isoriginaltype) {
        spalnum = 0;
    }
    // Select a radar image based on sidebar type (DOS/Gold) and player side
    // (Good/Bad/Other)
    if ((player->getSide()&~PS_MULTI) == PS_BAD) {
        side = isoriginaltype?3:0;
    } else if ((player->getSide()&~PS_MULTI) == PS_GOOD) {
        side = isoriginaltype?4:1;
    } else {
        side = isoriginaltype?5:2;
    }

    if (game.config.gametype == GAME_RA) {
        // RA follows (Gold names) * 3 and (DOS names) * 3
        side += 6;
    }

    radarname = radarnames[side];
    radaranim = 0;
    try {
        radarlogo = pc::imgcache->loadImage(radarname, scaleq);
    } catch (ImageNotFound&) {
        /// @TODO This problem should ripple up to the "game detection" layer
        // so it can try again from scratch with a different set of data files.
        game.log << "Hmm.. managed to misdetect sidebar type" << endl;
        try {
            // Switch between Gold and DOS
            if (side < 3) {
                side += 3;
            } else {
                side -= 3;
            }
            radarname = radarnames[side];
            radarlogo = pc::imgcache->loadImage(radarname, scaleq);
            isoriginaltype = !isoriginaltype;
        } catch (ImageNotFound&) {
            game.log << "Unable to load the radar-image! (Maybe you run c&c gold but have forgoten updatec.mix?)" << endl;
            throw SidebarError();
        }
    }

    SDL_Surface *radar = pc::imgcache->getImage(radarlogo).image;
    radarlocation.x = 0;  radarlocation.y = 0;
    radarlocation.w = radar->w;  radarlocation.h = radar->h;

    /// @TODO Move these values into config
    steps = ((game.config.gametype == GAME_RA)?54:108);

    sbarlocation.x = sbarlocation.y = sbarlocation.w = sbarlocation.h = 0;

    std::fill(greyFixed, greyFixed+256, false);

    pc::sidebar = this;
    SetupButtons(height);
}

Sidebar::~Sidebar()
{
    unsigned int i;

    SDL_FreeSurface(sbar);

    vector<shared_ptr<SidebarButton> > tmpbuttons;
    tmpbuttons.swap(buttons);

    for (i=0; i < uniticons.size(); ++i) {
        delete[] uniticons[i];
    }

    for (i=0; i < structicons.size(); ++i) {
        delete[] structicons[i];
    }
}

/**
 * @returns whether the sidebar's visibility has changed.
 */
bool Sidebar::getVisChanged() {
    if (vischanged) {
        vischanged = false;
        return true;
    }
    return false;
}

void Sidebar::ToggleVisible()
{
    visible = !visible;
    vischanged = true;
}

void Sidebar::ReloadImages()
{
    // Free the sbar, so that it is redrawn on next update
    SDL_FreeSurface(sbar);
    sbar = NULL;
    sbarlocation.x = sbarlocation.y = sbarlocation.w = sbarlocation.h = 0;

    // Reload the font we use
    gamefnt->reload();

    // Reload all the buttons
    for_each(buttons.begin(), buttons.end(), boost::mem_fn(&Sidebar::SidebarButton::ReloadImage));

    // Invalidate grey fix table
    std::fill(greyFixed, greyFixed+256, false);
}

SDL_Surface *Sidebar::getSidebarImage(SDL_Rect location)
{
    SDL_Rect dest, src;
    SDL_Surface *temp;

    if (location.w == sbarlocation.w && location.h == sbarlocation.h)
        return sbar;

    SDL_FreeSurface(sbar);

    temp = SDL_CreateRGBSurface(SDL_SWSURFACE, location.w, location.h, 16, 0, 0, 0, 0);
    sbar = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    location.x = 0;
    location.y = 0;

    /// @TODO HACK
    if (isoriginaltype || game.config.gametype == GAME_RA) {
        SDL_FillRect(sbar, &location, SDL_MapRGB(sbar->format, 0xa0, 0xa0, 0xa0));
    } else {
        try {
            // Get the index of the btexture
            unsigned int idx = pc::imgcache->loadImage("btexture.shp", scaleq);

            // Get the SDL_Surface for this texture
            SDL_Surface* texture = pc::imgcache->getImage(idx).image;

            dest.x = 0;
            dest.y = 0;
            dest.w = location.w;
            dest.h = texture->h;
            src.x = 0;
            src.y = 0;
            src.w = location.w;
            src.h = texture->h;
            for (dest.y = 0; dest.y < location.h; dest.y += dest.h) {
                SDL_BlitSurface(texture, &src, sbar, &dest);
            }
        } catch (ImageNotFound&) {
            game.log << "Unable to load the background texture" << endl;
            SDL_FillRect(sbar, &location, SDL_MapRGB(sbar->format, 0xa0, 0xa0, 0xa0));
        }
    }
    sbarlocation = location;

    if (!Input::isMinimapEnabled()) {
        SDL_Surface* radar = pc::imgcache->getImage(radarlogo).image;
        SDL_BlitSurface(radar, NULL, sbar, &radarlocation);
    }

    for (char x = buttons.size()-1;x>=0;--x) {
        DrawButton(x);
    }

    return sbar;
}

void Sidebar::AddButton(unsigned short x, unsigned short y, const char* fname, unsigned char f, unsigned char pal)
{
    shared_ptr<SidebarButton> t(new SidebarButton(x, y, fname, f, theatre, pal));
    buttons.push_back(t);
    vischanged = true;
}

void Sidebar::SetupButtons(unsigned short height)
{
    const char* tmpname;
    unsigned short scrollbase;
    unsigned char t;

    unsigned int startoffs = tablocation.h + radarlocation.h;

    SHPImage *strip;

    tmpname = VFS_getFirstExisting(3,"stripna.shp","hstrip.shp","strip.shp");
    if (tmpname == 0) {
        game.log << "Unable to find strip images for sidebar, exiting" << endl;
        throw SidebarError();
    }
    try {
        strip = new SHPImage(tmpname, scaleq);
    } catch (ImageNotFound&) {
        game.log << "Unable to load strip images for sidebar, exiting" << endl;
        throw SidebarError();
    }

    geom.bh = strip->getHeight();
    geom.bw = strip->getWidth();

    delete strip;

    if (geom.bh > 100)
        geom.bh = geom.bh>>2;

    buildbut = ((height-startoffs)/geom.bh)-2;
    startoffs += geom.bh;

    scrollbase = startoffs + geom.bh*buildbut;
    AddButton(10+geom.bw,scrollbase,"stripup.shp",sbo_scroll|sbo_unit|sbo_up,0); // 0
    AddButton(10,scrollbase,"stripup.shp",sbo_scroll|sbo_structure|sbo_up,0); // 1

    AddButton(10+geom.bw+(geom.bw>>1),scrollbase,"stripdn.shp",sbo_scroll|sbo_unit|sbo_down,0); // 2
    AddButton(10+(geom.bw>>1),scrollbase,"stripdn.shp",sbo_scroll|sbo_structure|sbo_down,0); // 3

    // The order in which the AddButton calls are made MUST be preserved
    // Two loops are made so that all unit buttons and all structure buttons
    // are grouped contiguously (4,5,6,7,...) compared to (4,6,8,10,...)

    for (t=0;t<buildbut;++t) {
        if (game.config.gametype == GAME_RA) AddButton(10+geom.bw,startoffs+geom.bh*t,"stripna.shp",sbo_build|sbo_unit,0);
          else AddButton(10+geom.bw,startoffs+geom.bh*t,"strip.shp",sbo_build|sbo_unit,0);
    }
    for (t=0;t<buildbut;++t) {
        if (game.config.gametype == GAME_RA) AddButton(10,startoffs+geom.bh*t,"stripna.shp",sbo_build|sbo_structure,spalnum);
          else AddButton(10,startoffs+geom.bh*t,"strip.shp",sbo_build|sbo_structure,spalnum);
    }

    UpdateAvailableLists();
    UpdateIcons();
    if (uniticons.empty() && structicons.empty()) {
        visible = false;
        vischanged = true;
    }
}

unsigned char Sidebar::getButton(unsigned short x,unsigned short y)
{
    SDL_Rect tmp;

    for (unsigned char i=0;i<buttons.size();++i) {
        tmp = buttons[i]->getRect();
        if (x>=tmp.x && y>=tmp.y && x<(tmp.x+tmp.w) && y<(tmp.y+tmp.h)) {
            return i;
        }
    }
    return 255;
}

void Sidebar::DrawButton(unsigned char index)
{
    if (sbar == 0) {
        /// @TODO Ensure we don't actually get called when this is the case
        return;
    }
    SDL_Rect dest = buttons[index]->getRect();
    SDL_FillRect(sbar, &dest, 0x0);
    unsigned char func = buttons[index]->getFunction();

    // Blit the button's image onto the sidebar surface
    SDL_Surface *temp = buttons[index]->getSurface();
    SDL_BlitSurface(temp, NULL, sbar, &dest);

    vischanged = true;
    // Skip scroll buttons
    if (index < 4) {
        return;
    }
    // Calculate which icon was clicked on
    unsigned char offset = index - 4 + // First four buttons are scroll buttons
                   ((func&sbo_unit)
                    ? unitoff  // Unit buttons are created first
                    : (structoff-buildbut) // See comment around line 265
                   );
    vector<char*>& icons = ((func&sbo_unit)
            ?(uniticons)
            :(structicons));

    if (offset >= icons.size()) {
        return;
    }

    // Extract type name from icon name, e.g. NUKE from NUKEICON.SHP
    unsigned int length = strlen(icons[offset])-8; if (length>13) length = 13;
    string name(icons[offset], length);

    static const char* stat_mesg[] = {
        "???",  // BQ_INVALID
        "",     // BQ_EMPTY
        "",     // BQ_RUNNING
        "hold", // BQ_PAUSED
        "ready" // BQ_READY
    };
    static struct {unsigned int x,y;} stat_pos[5];
    static bool posinit = false;
    if (!posinit) {
        for (unsigned char i = 0; i < 5; ++i) {
            // Here we assume that all sidebar icons are the same size
            // (which is pretty safe)
            stat_pos[i].x = (dest.w - getFont()->calcTextWidth(stat_mesg[i])) >> 1;
            stat_pos[i].y = dest.h - 15;
        }
        posinit = true;
    }
    ConStatus status;
    unsigned char quantity, progress, imgnum;

    UnitOrStructureType* type;
    if (func&sbo_unit) {
        type = p::uspool->getUnitTypeByName(name.c_str());
    } else {
        type = (UnitOrStructureType*)p::uspool->getStructureTypeByName(name.c_str());
    }
    if (0 == type) {
        throw runtime_error("Asking for \""+name+"\" resulted in a null pointer");
    }
    status = player->getStatus(type, &quantity, &progress);
    if (BQ_INVALID == status) {
        getFont()->drawText(stat_mesg[status], sbar, dest.x + stat_pos[status].x, dest.y + stat_pos[status].y);
        // Grey out invalid items for prettyness
        DrawClock(index, 0);
        return;
    }
    imgnum = (steps*progress)/100;
    if (100 != progress) {
        DrawClock(index, imgnum);
    }
    if (quantity > 0) {
        char tmp[4];
        sprintf(tmp, "%i", quantity);
        getFont()->drawText(tmp, sbar, dest.x+3, dest.y+3);
    }
    /// @TODO This doesn't work when you immediately pause after starting to
    // build (but never draws the clock for things not being built).
    if (progress > 0) {
        getFont()->drawText(stat_mesg[status], sbar, dest.x + stat_pos[status].x, dest.y + stat_pos[status].y);
    }
}

void Sidebar::FixGrey(SDL_Surface* gr, unsigned char num)
{
    if (greyFixed[num]) {
        return;
    }

    SDL_LockSurface(gr);
    if(gr->format->BytesPerPixel == 4) {
        unsigned int *pixels = (unsigned int *)gr->pixels;
        unsigned int size = ((gr->h-1) * (gr->pitch >> 2)) + gr->w-1;
        replace(pixels, pixels+size, 0xa800U, 0x10101U);
    } else if(gr->format->BytesPerPixel == 2) {
        unsigned short *pixels = (unsigned short *)gr->pixels;
        unsigned int size = ((gr->h-1) * (gr->pitch >> 1)) + gr->w-1;
        replace(pixels, pixels+size, 0x540U, 0x21U);
    }
    SDL_UnlockSurface(gr);
    greyFixed[num] = true;
}

void Sidebar::DrawClock(unsigned char index, unsigned char imgnum) {
    unsigned int num = 0;
    try {
        // @TODO Move this check to config object
        if (game.config.gametype == GAME_RA || isoriginaltype) {
            num = pc::imgcache->loadImage("clock.shp");
        } else {
            num = pc::imgcache->loadImage("hclock.shp");
        }
    } catch (ImageNotFound& e) {
        game.log << "Unable to load clock image!" << endl;
        return;
    }
    num += imgnum;
    num |= spalnum<<11;

    ImageCacheEntry& ice = pc::imgcache->getImage(num);
    SDL_Surface* gr = ice.image;
    FixGrey(gr, imgnum);
    SDL_SetAlpha(gr, SDL_SRCALPHA, 128);

    SDL_Rect dest = buttons[index]->getRect();

    SDL_BlitSurface(gr, NULL, sbar, &dest);
}

/** Event handler for clicking buttons.
 *
 * @bug This is an evil hack that should be replaced by something more flexible.
 */
/// @TODO Bleh, this function needs loving.
void Sidebar::ClickButton(unsigned char index, char* unitname, createmode_t* createmode) {
    unsigned char f = buttons[index]->getFunction();
    *createmode = CM_INVALID;
    switch (f&0x3) {
    case 0:
        return;
        break;
    case 1: // build
        Build(index - 3 - ((f&sbo_unit)?0:buildbut),(f&sbo_unit),unitname, createmode);
        break;
    case 2: // scroll
        DownButton(index);
        ScrollBuildList((f&sbo_up), (f&sbo_unit));
        break;
    default:
        // unreachable
        break;
    }
}

/// @TODO Bleh, this function needs loving.
void Sidebar::Build(unsigned char index, unsigned char type, char* unitname, createmode_t* createmode) {
    unsigned char* offptr;
    std::vector<char*>* vecptr;

    if (type) {
        offptr = &unitoff;
        vecptr = &uniticons;
    } else {
        offptr = &structoff;
        vecptr = &structicons;
        *createmode = CM_STRUCT;
    }

    if ( (unsigned)(*vecptr).size() > ((unsigned)(*offptr)+index - 1) ) {
        strncpy(unitname,(*vecptr)[(*offptr+index-1)],13);
        unitname[strlen((*vecptr)[(*offptr+index-1)])-8] = 0x0;
    } else {
        // Out of range
        return;
    }

    if (*createmode != CM_STRUCT) {
        // createmode was set to CM_INVALID in the caller of this function
        UnitType* utype = p::uspool->getUnitTypeByName(unitname);
        if (0 != utype) {
            *createmode = (createmode_t)utype->getType();
        }
    }
}

void Sidebar::ScrollBuildList(unsigned char dir, unsigned char type) {
    unsigned char* offptr;
    std::vector<char*>* vecptr;

    if (type) {
        offptr = &unitoff;
        vecptr = &uniticons;
    } else {
        offptr = &structoff;
        vecptr = &structicons;
    }

    if (dir) { //up
        if (*offptr>0) {
            --(*offptr);
        }
    } else { // down
        if ((unsigned)(*offptr+buildbut) < (unsigned)vecptr->size()) {
            ++(*offptr);
        }
    }

    UpdateIcons();
}

void Sidebar::UpdateSidebar() {
    UpdateAvailableLists();
    UpdateIcons();
}

/** Rebuild the list of icons that are available.
 *
 * @bug Newer items should be appended, although with some grouping (i.e. keep
 * infantry at top, vehicles, aircraft, boats, then superweapons).
 */
void Sidebar::UpdateAvailableLists() {
    std::vector<const char*> units_avail, structs_avail;
    char* nametemp;
    bool played_sound;
    unsigned char prev_units_avail, prev_structs_avail;
    unsigned int i;
    played_sound = false;
    units_avail = p::uspool->getBuildableUnits(player);
    structs_avail = p::uspool->getBuildableStructures(player);
    prev_units_avail = uniticons.size();
    prev_structs_avail = structicons.size();
    if (units_avail.size() != uniticons.size()) {
        unitoff = 0;
        if (units_avail.size() > uniticons.size()) {
            played_sound = true;
            pc::sfxeng->PlaySound("newopt1.aud");
        }
        for (i=0;i<uniticons.size();++i) {
            delete[] uniticons[i];
        }
        uniticons.resize(0);
        for (i=0;i<units_avail.size();++i) {
            nametemp = new char[13];
            memset(nametemp,0x0,13);
            sprintf(nametemp,"%sICON.SHP",units_avail[i]);
            uniticons.push_back(nametemp);
        }
    }
    if (structs_avail.size() != structicons.size()) {
        structoff = 0;
        if ((structs_avail.size() > structicons.size())&&!played_sound) {
            pc::sfxeng->PlaySound("newopt1.aud");
        }
        for (i=0;i<structicons.size();++i) {
            delete[] structicons[i];
        }
        structicons.resize(0);
        for (i=0;i<structs_avail.size();++i) {
            nametemp = new char[13];
            memset(nametemp,0x0,13);
            sprintf(nametemp,"%sICON.SHP",structs_avail[i]);
            structicons.push_back(nametemp);
        }
    }
    visible = (visible || uniticons.size() > prev_units_avail ||
            structicons.size() > prev_structs_avail);
    if (uniticons.empty() && structicons.empty()) {
        visible = false;
    }
    vischanged = true;
}

/** Sets the images of the visible icons, having scrolled.
 * @TODO Provide a way to only update certain icons
 */
void Sidebar::UpdateIcons() {
    unsigned char i;

    for (i=0;i<buildbut;++i) {
        if ((unsigned)(i+unitoff)>=(unsigned)uniticons.size()) {
            if (game.config.gametype == GAME_RA) buttons[4+i]->ChangeImage("stripna.shp");
            else buttons[4+i]->ChangeImage("strip.shp");
        } else {
            buttons[4+i]->ChangeImage(uniticons[i+unitoff]);
        }
    }
    for (i=0;i<buildbut;++i) {
        if ((unsigned)(i+structoff)>=(unsigned)structicons.size()) {
            if (game.config.gametype == GAME_RA) buttons[buildbut+4+i]->ChangeImage("stripna.shp");
            else buttons[buildbut+4+i]->ChangeImage("strip.shp");
        } else {
            buttons[buildbut+4+i]->ChangeImage(structicons[i+structoff]);
        }
    }

    for (char x=buttons.size()-1;x>=0;--x) {
        DrawButton(x);
    }
}

void Sidebar::DownButton(unsigned char index)
{
    if (index>3)
        return; // not a scroll button

    buttondown = index;

    bd = true;
    if (index == 0 || index == 1)
        buttons[index]->ChangeImage("stripup.shp",1);
    else
        buttons[index]->ChangeImage("stripdn.shp",1);
    UpdateIcons();

}

void Sidebar::ResetButton()
{
    if (!bd)
        return;
    bd = false;

    if (buttondown < 2)
        buttons[buttondown]->ChangeImage("stripup.shp",0);
    else
        buttons[buttondown]->ChangeImage("stripdn.shp",0);
    buttondown = 0;
    UpdateIcons();
}

void Sidebar::StartRadarAnim(unsigned char mode, bool* minienable)
{
    if (radaranimating == false && radaranim == NULL && sbar != NULL) {
        radaranimating = true;
        radaranim = new RadarAnimEvent(mode, minienable, radarlogo);
    }
}

void Sidebar::ScrollSidebar(bool scrollup)
{
    ScrollBuildList(scrollup, 0);
    ScrollBuildList(scrollup, 1);
}

Sidebar::SidebarButton::SidebarButton(short x, short y, const char* picname,
        unsigned char f, const char* theatre, unsigned char pal)
 : pic(0), function(f), palnum(pal), theatre(theatre), using_fallback(false)
{
    picloc.x = x;
    picloc.y = y;
    ChangeImage(picname);
}

void Sidebar::SidebarButton::ChangeImage(const char* fname)
{
    ChangeImage(fname,0);
}

SDL_Surface* Sidebar::SidebarButton::Fallback(const char* fname)
{
    SDL_Surface* ret;
    unsigned int width, height;
    unsigned int slen = strlen(fname);
    char* iname = new char[slen-7];

    using_fallback = true; // Ensures that the surface created gets destroyed later.
    fallbackfname = fname;

    strncpy(iname,fname,slen-8);
    iname[slen-8] = 0;
    width = pc::sidebar->getGeom().bw;
    height = pc::sidebar->getGeom().bh;
    ret = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCCOLORKEY, width, height, 16, 0, 0, 0, 0);
    SDL_FillRect(ret, NULL, 0);
    pc::sidebar->getFont()->drawText(iname,ret,0,0);
    delete[] iname;
    return ret;
}

void Sidebar::SidebarButton::ChangeImage(const char* fname, unsigned char number)
{
    const char* name;
    char goldname[32];
    unsigned int slen = strlen(fname);

    if (using_fallback) {
        SDL_FreeSurface(pic);
        using_fallback = false;
    }

    if (pc::sidebar->isOriginalType() || game.config.gametype == GAME_RA) {
        name = fname;
    } else {
        if (slen>8 && strcasecmp("icon.shp", fname+slen-8)==0) {
            strcpy(goldname, fname);
            goldname[slen-6] = 'N';
            goldname[slen-5] = 'H';
            goldname[slen-3] = theatre[0];
            goldname[slen-2] = theatre[1];
            goldname[slen-1] = theatre[2];
        } else {
            sprintf(goldname, "H%s", fname);
        }
        name = goldname;
    }

    try {
        picnum = pc::imgcache->loadImage(name);
        picnum += number;
        picnum |= palnum<<11;
        ImageCacheEntry& ICE = pc::imgcache->getImage(picnum);
        pic = ICE.image;
    } catch(ImageNotFound&) {
        /* This is only a temporary fix; the real solution is to add a creation
         * function to the imagecache, which would ahandle creation and would mean
         * the imagecache would be responsible for deletion.  A nice side effect is
         * that those changes will be acompanied with a way to index created images
         * so we only create the fallback images once. */
        pic = Fallback(fname);
    }

    picloc.w = pic->w;
    picloc.h = pic->h;

    if (picloc.h > 100) {
        picloc.h >>=2;
        pic->h >>= 2;
    }
}

void Sidebar::SidebarButton::ReloadImage()
{
    //If we are using fallback we must redraw
    if (using_fallback) {
        SDL_FreeSurface(pic);
        pic = Fallback(fallbackfname);
    } else {
        //else the image cache takes care of it
        pic = pc::imgcache->getImage(picnum).image;
    }
}

Sidebar::SidebarButton::~SidebarButton()
{
    if (using_fallback) {
        SDL_FreeSurface(pic);
        using_fallback = false;
    }
}


Sidebar::RadarAnimEvent::RadarAnimEvent(unsigned char mode, bool* minienable, unsigned int radar)
    : ActionEvent(1), mode(mode), minienable(minienable), radar(radar)
{
    switch (mode) {
    case 0:
        frame = 0;
        framend = 20;
        break;
    case 1:
        //*minienable = false;
        frame = 20;
        framend = 30;
        break;
    default:
        frame = 0;
    }
    p::aequeue->scheduleEvent(this);
}

void Sidebar::RadarAnimEvent::run()
{
    SDL_Rect *dest = &pc::sidebar->radarlocation;
    SDL_Surface *radarFrame;

    if (frame <= framend) {

        //If the sidebar is null don't even bother
        if (pc::sidebar->sbar != NULL) {
            // Draw radar loading frame
            SDL_FillRect(pc::sidebar->sbar, dest, SDL_MapRGB(pc::sidebar->sbar->format, 0x0a, 0x0a, 0x0a));
            radarFrame = pc::imgcache->getImage(radar, frame).image;
            SDL_BlitSurface( radarFrame, NULL, pc::sidebar->sbar, dest);
        }

        ++frame;
        p::aequeue->scheduleEvent(this);
    } else {
        if (mode == 1) {
            if (pc::sidebar->sbar != NULL) {
                // Draw no radar logo
                radarFrame = pc::imgcache->getImage(radar).image;
                SDL_BlitSurface( radarFrame, NULL, pc::sidebar->sbar, dest);
            }
            pc::sidebar->radaranim = NULL;
        } else {
            if (pc::sidebar->sbar != NULL) {
                // Draw grey box where radar was
                SDL_FillRect(pc::sidebar->sbar, dest, SDL_MapRGB(pc::sidebar->sbar->format, 0x0a, 0x0a, 0x0a));
            }
            *minienable = true;
            pc::sidebar->radaranim = NULL;
        }
        pc::sidebar->radaranimating = false;
        delete this;
        return;
    }
}
