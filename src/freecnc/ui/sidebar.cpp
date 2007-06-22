#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#include <boost/mem_fn.hpp>

#include "../game/game_public.h"
#include "../sound/sound_public.h"
#include "input.h"
#include "sidebar.h"

using std::string;
using std::runtime_error;
using std::vector;
using std::replace;

class ImageCache;

namespace
{
    enum {
        IMG_TABS, IMG_STRIP, IMG_STRIP_UP, IMG_STRIP_DOWN,
        IMG_RADAR_BADGUY, IMG_RADAR_GOODGUY, IMG_RADAR_SPECIAL,
        IMG_CLOCK,
        IMG_REPAIR, IMG_SELL, IMG_MAP,
        IMG_TOP, IMG_BOTTOM, IMG_POWER_INDICATOR, IMG_POWER_BAR,
        IMG_INVALID
    };
        
    // Unused image: side1na.shp is the Radar frame
    const char* sidebar_filenames[5][IMG_INVALID] = {
        {
            0, 0, 0, 0, 0, 0, 0, 0
        },
        {
            "tabs.shp",  "strip.shp",  "stripup.shp", "stripdn.shp",
            "radar.nod", "radar.gdi",  "radar.jp",
            "clock.shp",
            "repair.shp", "sell.shp", "map.shp",
            "side1.shp", "side2.shp", "power.shp", "pwrbar.shp"
        },
        {
            "htabs.shp", "hstrip.shp", "hstripup.shp", "hstripdn.shp",
            "hradar.nod", "hradar.gdi", "hradar.jp",
            "hclock.shp",
            "hrepair.shp", "hsell.shp", "hmap.shp", 
            "hside1.shp", "hside2.shp", "hpower.shp", "hpwrbar.shp"
        },
        {
            "tabs.shp",  "stripna.shp", "stripup.shp", "stripdn.shp",
            "ussrradr.shp", "natoradr.shp", "natoradr.shp",
            "clock.shp",
            "repair.shp", "sell.shp","map.shp",
            "side2na.shp", "side3na.shp", "power.shp", "powerbar.shp"
        },
        {
            "tabs.shp",  "stripus.shp", "stripup.shp", "stripdn.shp",
            "ussrradr.shp", "natoradr.shp", "natoradr.shp",
            "clock.shp",
            "repair.shp", "sell.shp","map.shp",
            "side2us.shp", "side3us.shp", "power.shp", "powerbar.shp"
        }
    };
    const char** filenames = sidebar_filenames[0]; // gets set to sidebar_filenames[sidebar_type]
}

/**
 * @param pl local player object
 * @param height height of screen in pixels
 * @param theatre terrain type of the theatre
 *      TD: desert, temperate and winter
 *      RA: temperate and snow
 */
Sidebar::Sidebar(Player* pl, int height, const char* theatre)
    : height(height), unit_palette(0), structure_palette(0), radarlogo(0),
    tab(0), sbar(0), gamefnt(new Font("scorefnt.fnt")), visible(true),
    vischanged(true), invalidated(true), theatre(theatre), buttondown(0),
    bd(false), radaranimating(false), unitoff(0), structoff(0), player(pl),
    scaleq(-1)
{
    load_images();

    init_radar();

    init_surface();

    blit_static_images();

    std::fill(grey_fixed, grey_fixed+256, false);

    pc::sidebar = this;

    setup_buttons();
}

void Sidebar::load_images()
{
    if (game.config.gametype == GAME_TD) {
        if (game.vfs.open(sidebar_filenames[GOLD][IMG_TABS])) {
            sidebar_type = GOLD;
        } else if (game.vfs.open(sidebar_filenames[DOS][IMG_TABS])) {
            sidebar_type = DOS;
            structure_palette = player->getStructpalNum();
            unit_palette = player->getUnitpalNum();
        } else {
            throw runtime_error("Unable to find the tab images! (Missing updatec.mix?)");
        }
    } else {
        sidebar_type = REDALERT_NATO;
        // Duplicate check from init_radar for now...
        if ((player->getSide()&~PS_MULTI) == PS_BAD) {
            sidebar_type = REDALERT_USSR;
        }
    }
    filenames = sidebar_filenames[sidebar_type];
    tab = pc::imgcache->loadImage(filenames[IMG_TABS], scaleq);

    SDL_Surface* tmp = pc::imgcache->getImage(tab).image;

    tablocation.x = 0;
    tablocation.y = 0;
    tablocation.w = tmp->w;
    tablocation.h = tmp->h;
}

void Sidebar::init_radar()
{
    int radar_index;
    // Select a radar image based on sidebar type (DOS/Gold) and player side
    // (Good/Bad/Other)
    if ((player->getSide()&~PS_MULTI) == PS_BAD) {
        radar_index = IMG_RADAR_BADGUY;
    } else if ((player->getSide()&~PS_MULTI) == PS_GOOD) {
        radar_index = IMG_RADAR_GOODGUY;
    } else {
        radar_index = IMG_RADAR_SPECIAL;
    }

    radarlogo = pc::imgcache->loadImage(filenames[radar_index], scaleq);

    SDL_Surface* radar = pc::imgcache->getImage(radarlogo).image;
    radarlocation.x = 0;
    radarlocation.y = 0;
    radarlocation.w = radar->w;
    radarlocation.h = radar->h;
    steps = ((game.config.gametype == GAME_RA)?54:108);
}

void Sidebar::init_surface()
{
    SDL_Surface* temp = SDL_CreateRGBSurface(SDL_SWSURFACE, tablocation.w, height, 16, 0, 0, 0, 0);
    sbar = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
}

void Sidebar::blit_static_images()
{
    if (sidebar_type == DOS) {
        SDL_FillRect(sbar, NULL, SDL_MapRGB(sbar->format, 0xa0, 0xa0, 0xa0));
    } else if (sidebar_type == GOLD) {
        unsigned int idx = pc::imgcache->loadImage("btexture.shp", scaleq);
        SDL_Surface* texture = pc::imgcache->getImage(idx, 1).image;
        SDL_Rect dest = {0, 0, tablocation.w, texture->h};
        for (dest.y = 0; dest.y < height; dest.y += dest.h) {
            SDL_BlitSurface(texture, NULL, sbar, &dest);
        }
    }

    //// Draw the sidebar frames
    try { // Temp try/catch
        unsigned int idx = pc::imgcache->loadImage(filenames[IMG_TOP], scaleq);
        SDL_Surface* texture = pc::imgcache->getImage(idx, 1).image;
        SDL_Rect dest = {0, tablocation.h + radarlocation.h, tablocation.w, texture->h};
        SDL_BlitSurface(texture, NULL, sbar, &dest);
        dest.y += dest.h;
        idx = pc::imgcache->loadImage(filenames[IMG_BOTTOM], scaleq);
        texture = pc::imgcache->getImage(idx, 1).image;
        dest.h = texture->h;
        SDL_BlitSurface(texture, NULL, sbar, &dest);
    } catch (ImageNotFound& e) {
        // DOS version needs its TOP and BOTTOM images painting
    }


    //// Draw the repair, sell and map buttons
    const int yoffsets[5] = {0, 0, 2, 0, 0};
    const int xoffsets[5][3] = {{0, 0, 0}, {2, 36, 58}, {4, 57, 110}, {0, 0, 0}, {0, 0, 0}};

    const int* xoffset = xoffsets[sidebar_type];

    unsigned int idx = pc::imgcache->loadImage(filenames[IMG_REPAIR], scaleq);
    SDL_Surface* texture = pc::imgcache->getImage(idx, 0).image;
    SDL_Rect dest = {xoffset[0],
        yoffsets[sidebar_type] + tablocation.h + radarlocation.h,
        texture->w, texture->h};
    SDL_BlitSurface(texture, NULL, sbar, &dest);

    idx = pc::imgcache->loadImage(filenames[IMG_SELL], scaleq);
    texture = pc::imgcache->getImage(idx, 0).image;
    dest.x = xoffset[1];
    dest.w = texture->w;
    dest.h = texture->h;
    SDL_BlitSurface(texture, NULL, sbar, &dest);

    idx = pc::imgcache->loadImage(filenames[IMG_MAP], scaleq);
    texture = pc::imgcache->getImage(idx, 0).image;
    dest.x = xoffset[2];
    dest.w = texture->w;
    dest.h = texture->h;
    SDL_BlitSurface(texture, NULL, sbar, &dest);


    //// Draw the power bar
    if (sidebar_type == DOS) {
        // No powerbar for DOS version yet...
        return;
    }
    idx = pc::imgcache->loadImage(filenames[IMG_POWER_BAR], scaleq);
    texture = pc::imgcache->getImage(idx, 3).image;
    dest.x = 0;
    dest.y = 274;
    dest.w = texture->w;
    dest.h = texture->h;
    SDL_BlitSurface(texture, NULL, sbar, &dest);

}

Sidebar::~Sidebar()
{
    unsigned int i;

    SDL_FreeSurface(sbar);

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
bool Sidebar::get_vischanged() {
    if (vischanged) {
        vischanged = false;
        return true;
    }
    return false;
}

void Sidebar::toggle_visible()
{
    visible = !visible;
    vischanged = true;
}

void Sidebar::reload_images()
{
    invalidated = true;

    // Reload the font we use
    gamefnt->reload();

    // Reload all the buttons
    for_each(buttons.begin(), buttons.end(), boost::mem_fn(&Sidebar::SidebarButton::reload_image));

    // Invalidate grey fix table
    std::fill(grey_fixed, grey_fixed+256, false);
}

SDL_Surface *Sidebar::get_sidebar_image(SDL_Rect location)
{
    if (!invalidated) {
        return sbar;
    }

    if (!Input::isMinimapEnabled()) {
        SDL_Surface* radar = pc::imgcache->getImage(radarlogo).image;
        SDL_BlitSurface(radar, NULL, sbar, &radarlocation);
    }

    for (int x = buttons.size() - 1 ; x >= 0; --x) {
        draw_button(x);
    }

    invalidated = false;

    return sbar;
}

void Sidebar::add_button(unsigned short x, unsigned short y, const char* fname, unsigned char f, unsigned char pal)
{
    shared_ptr<SidebarButton> t(new SidebarButton(x, y, fname, f, theatre, pal, sidebar_type));
    buttons.push_back(t);
    vischanged = true;
}

void Sidebar::setup_buttons()
{
    {
        SHPImage strip(filenames[IMG_STRIP], scaleq);
        geom.bh = strip.getHeight();
        geom.bw = strip.getWidth();
    }
    if (sidebar_type != DOS) {
        // Non DOS strips appear as a four
        geom.bh = geom.bh / 4;
    }

    unsigned int startoffs = tablocation.h + radarlocation.h;

    int sx, sy, ux, uy;

    if (sidebar_type == DOS) {
        buildbut = ((height-startoffs)/geom.bh)-2;
        sx = 10;
        sy = startoffs + 10;
        ux = 10 + geom.bw;
        uy = sy;
    } else {
        buildbut = 4;
        sx = 20;
        sy = startoffs + 22;
        ux = 90;
        uy = sy;
    }

    int scroll_y = sy + geom.bh * buildbut;
    add_button(ux, scroll_y, filenames[IMG_STRIP_UP], sbo_scroll|sbo_unit|sbo_up, 0); // 0
    add_button(sx, scroll_y, filenames[IMG_STRIP_UP], sbo_scroll|sbo_structure|sbo_up, 0); // 1

    add_button(ux + (geom.bw / 2), scroll_y, filenames[IMG_STRIP_DOWN], sbo_scroll|sbo_unit|sbo_down, 0); // 2
    add_button(sx + (geom.bw / 2), scroll_y, filenames[IMG_STRIP_DOWN], sbo_scroll|sbo_structure|sbo_down, 0); // 3

    // The order in which the add_button calls are made MUST be preserved
    // Two loops are made so that all unit buttons and all structure buttons
    // are grouped contiguously (4,5,6,7,...) compared to (4,6,8,10,...)

    for (int i = 0;i < buildbut; ++i) {
        add_button(ux, uy + geom.bh * i, filenames[IMG_STRIP], sbo_build|sbo_unit, unit_palette);
    }
    for (int i = 0; i < buildbut; ++i) {
        add_button(sx, sy + geom.bh * i, filenames[IMG_STRIP], sbo_build|sbo_structure, structure_palette);
    }

    update_available_lists();
    update_icons();
    if (uniticons.empty() && structicons.empty()) {
        visible = false;
        vischanged = true;
    }
}

unsigned char Sidebar::get_button(unsigned short x,unsigned short y)
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

void Sidebar::draw_button(unsigned char index)
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
            stat_pos[i].x = (dest.w - get_font()->calcTextWidth(stat_mesg[i])) >> 1;
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
        get_font()->drawText(stat_mesg[status], sbar, dest.x + stat_pos[status].x, dest.y + stat_pos[status].y);
        // Grey out invalid items for prettyness
        draw_clock(index, 0);
        return;
    }
    imgnum = (steps*progress)/100;
    if (100 != progress) {
        draw_clock(index, imgnum);
    }
    if (quantity > 0) {
        char tmp[4];
        sprintf(tmp, "%i", quantity);
        get_font()->drawText(tmp, sbar, dest.x+3, dest.y+3);
    }
    /// @TODO This doesn't work when you immediately pause after starting to
    // build (but never draws the clock for things not being built).
    if (progress > 0) {
        get_font()->drawText(stat_mesg[status], sbar, dest.x + stat_pos[status].x, dest.y + stat_pos[status].y);
    }
}

void Sidebar::fix_grey(SDL_Surface* gr, unsigned char num)
{
    if (grey_fixed[num]) {
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
    grey_fixed[num] = true;
}

void Sidebar::draw_clock(unsigned char index, unsigned char imgnum) {
    unsigned int num = 0;
    num = pc::imgcache->loadImage(filenames[IMG_CLOCK]);
    num += imgnum;
    num |= structure_palette<<11;

    ImageCacheEntry& ice = pc::imgcache->getImage(num);
    SDL_Surface* gr = ice.image;
    fix_grey(gr, imgnum);
    SDL_SetAlpha(gr, SDL_SRCALPHA, 128);

    SDL_Rect dest = buttons[index]->getRect();

    SDL_BlitSurface(gr, NULL, sbar, &dest);
}

/** Event handler for clicking buttons.
 *
 * @bug This is an evil hack that should be replaced by something more flexible.
 */
/// @TODO Bleh, this function needs loving.
void Sidebar::click_button(unsigned char index, char* unitname, createmode_t* createmode) {
    unsigned char f = buttons[index]->getFunction();
    *createmode = CM_INVALID;
    switch (f&0x3) {
    case 0:
        return;
        break;
    case 1: // build
        build(index - 3 - ((f&sbo_unit)?0:buildbut),(f&sbo_unit),unitname, createmode);
        break;
    case 2: // scroll
        down_button(index);
        scroll_build_list((f&sbo_up), (f&sbo_unit));
        break;
    default:
        // unreachable
        break;
    }
}

/// @TODO Bleh, this function needs loving.
void Sidebar::build(unsigned char index, unsigned char type, char* unitname, createmode_t* createmode) {
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

void Sidebar::scroll_build_list(unsigned char dir, unsigned char type) {
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

    update_icons();
}

void Sidebar::update_sidebar() {
    update_available_lists();
    update_icons();
}

/** Rebuild the list of icons that are available.
 *
 * @bug Newer items should be appended, although with some grouping (i.e. keep
 * infantry at top, vehicles, aircraft, boats, then superweapons).
 */
void Sidebar::update_available_lists() {
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
void Sidebar::update_icons() {
    unsigned char i;

    for (i=0;i<buildbut;++i) {
        if ((unsigned)(i+unitoff)>=(unsigned)uniticons.size()) {
            buttons[4+i]->change_image(filenames[IMG_STRIP]);
        } else {
            buttons[4+i]->change_image(uniticons[i+unitoff]);
        }
    }
    for (i=0;i<buildbut;++i) {
        if ((unsigned)(i+structoff)>=(unsigned)structicons.size()) {
            buttons[buildbut+4+i]->change_image(filenames[IMG_STRIP]);
        } else {
            buttons[buildbut+4+i]->change_image(structicons[i+structoff]);
        }
    }

    for (char x = buttons.size()-1;x>=0;--x) {
        draw_button(x);
    }
}

void Sidebar::down_button(unsigned char index)
{
    if (index>3)
        return; // not a scroll button

    buttondown = index;

    bd = true;
    if (index == 0 || index == 1)
        buttons[index]->change_image(filenames[IMG_STRIP_UP],1);
    else
        buttons[index]->change_image(filenames[IMG_STRIP_DOWN],1);
    update_icons();

}

void Sidebar::reset_button()
{
    if (!bd)
        return;
    bd = false;

    if (buttondown < 2)
        buttons[buttondown]->change_image(filenames[IMG_STRIP_UP],0);
    else
        buttons[buttondown]->change_image(filenames[IMG_STRIP_DOWN],0);
    buttondown = 0;
    update_icons();
}

void Sidebar::start_radar_anim(unsigned char mode, bool* minienable)
{
    if (radaranimating == false && !radaranim && sbar != NULL) {
        radaranimating = true;
        radaranim.reset(new RadarAnimEvent(mode, minienable, radarlogo));
        p::aequeue->scheduleEvent(radaranim);
    }
}

void Sidebar::scroll_sidebar(bool scrollup)
{
    scroll_build_list(scrollup, 0);
    scroll_build_list(scrollup, 1);
}

Sidebar::SidebarButton::SidebarButton(short x, short y, const char* picname,
        unsigned char f, const char* theatre, unsigned char pal, const Sidebar::SidebarType& st)
 : pic(0), function(f), palnum(pal), theatre(theatre), using_fallback(false), sidebar_type(st)
{
    picloc.x = x;
    picloc.y = y;
    change_image(picname);
}

void Sidebar::SidebarButton::change_image(const char* fname)
{
    change_image(fname,0);
}

SDL_Surface* Sidebar::SidebarButton::fallback(const char* fname)
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
    pc::sidebar->get_font()->drawText(iname,ret,0,0);
    delete[] iname;
    return ret;
}

void Sidebar::SidebarButton::change_image(const char* fname, unsigned char number)
{
    bool use_palette = true;

    if (using_fallback) {
        SDL_FreeSurface(pic);
        using_fallback = false;
    }

    string name(fname);
    if (sidebar_type == DOS) {
        // Do not apply unit palette to infantry icons
        if ((fname[0] == 'e' || fname[0] == 'E' && (fname[1] - '1' < 9))
            || strncasecmp("rmbo", fname, 4) == 0) {
            use_palette = false;
        }
    } else if (sidebar_type == GOLD) {
        size_t len = name.length();
        if (len > 8 && strcasecmp("icon.shp", fname + len - 8) == 0) {
            name[len - 6] = 'N';
            name[len - 5] = 'H';
            name[len - 3] = theatre[0];
            name[len - 2] = theatre[1];
            name[len - 1] = theatre[2];
        }
    }

    try {
        picnum = pc::imgcache->loadImage(name.c_str());
        picnum += number;
        if (use_palette) {
            picnum |= palnum<<11;
        }
        ImageCacheEntry& ICE = pc::imgcache->getImage(picnum);
        pic = ICE.image;
    } catch(ImageNotFound&) {
        /* This is only a temporary fix; the real solution is to add a creation
         * function to the imagecache, which would ahandle creation and would mean
         * the imagecache would be responsible for deletion.  A nice side effect is
         * that those changes will be acompanied with a way to index created images
         * so we only create the fallback images once. */
        pic = fallback(fname);
    }

    picloc.w = pic->w;
    picloc.h = pic->h;

    if (pic->h > 100) {
        // Non-DOS sidebar strips have four blocks
        picloc.h /= 4;
        pic->h /= 4;
    }
}

void Sidebar::SidebarButton::reload_image()
{
    //If we are using fallback we must redraw
    if (using_fallback) {
        SDL_FreeSurface(pic);
        pic = fallback(fallbackfname);
    } else {
        //else the image cache takes care of it
        pic = pc::imgcache->getImage(picnum).image;
        if (pic->h > 100) {
            // Non-DOS sidebar strips have four blocks
            pic->h /= 4;
        }
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
}

bool Sidebar::RadarAnimEvent::run()
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
        return true;
    }
    if (mode == 1) {
        if (pc::sidebar->sbar != NULL) {
            // Draw no radar logo
            radarFrame = pc::imgcache->getImage(radar).image;
            SDL_BlitSurface( radarFrame, NULL, pc::sidebar->sbar, dest);
        }
    } else {
        if (pc::sidebar->sbar != NULL) {
            // Draw grey box where radar was
            SDL_FillRect(pc::sidebar->sbar, dest, SDL_MapRGB(pc::sidebar->sbar->format, 0x0a, 0x0a, 0x0a));
        }
        *minienable = true;
    }
    pc::sidebar->radaranim.reset();
    pc::sidebar->radaranimating = false;
    return false;
}
