#ifndef _UI_INPUT_H
#define _UI_INPUT_H

#include "../freecnc.h"
#include "selection.h"

struct SDL_Rect;

class Player;
class StructureType;

class Input
{
public:
    Input(Uint16 screenwidth, Uint16 screenheight, SDL_Rect *maparea);
    ~Input();
    void handle();
    Uint8 shouldQuit()
    {
        return done;
    }
    static bool isMinimapEnabled()
    {
        return minimapEnabled;
    }
    static bool isDrawing()
    {
        return drawing;
    }
    static SDL_Rect getMarkRect()
    {
        return markrect;
    }
private:
    enum keymod {k_none = 0, k_shift = 1, k_ctrl = 2, k_alt = 4,
                 k_cs = 3, k_as = 5, k_ac = 6, k_acs = 7};

    enum mouseloc {m_none = 0, m_map = 1, m_sidebar = 2, m_tab = 3};

    enum actions {a_none = 0, a_place = 1, a_deploysuper = 2,
                  a_repair = 3, a_sell = 4};

    void updateMousePos();
    void clickMap(int mx, int my);
    void clickedTile(int mx, int my, Uint16* pos, Uint8* subpos);
    void clickSidebar(int mx, int my, bool rightbutton);
    void setCursorByPos(int mx, int my);
    void selectRegion();
    Uint16 checkPlace(int mx, int my);

    Uint16 width;
    Uint16 height;
    Uint8 done, donecount, finaldelay, gamemode;
    SDL_Rect *maparea;
    Uint16 tabwidth;
    Uint8 tilewidth;

    static bool drawing;
    static SDL_Rect markrect;
    static bool minimapEnabled;

    Player* lplayer;
    keymod kbdmod;
    mouseloc lmousedown, rmousedown;
    actions currentaction;
    bool rcd_scrolling;

    StructureType *placetype;
    char placename[13];
    bool placeposvalid;
    bool temporary_place_unit;

    Selection selected;

    int sx,sy;
};

#endif
