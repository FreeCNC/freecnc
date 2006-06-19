#ifndef _UI_SIDEBAR_H
#define _UI_SIDEBAR_H

#include "SDL.h"

#include "../freecnc.h"
#include "../renderer/renderer_public.h"
#include "../game/game_public.h"

class Font;
class Player;

enum createmode_t {
    CM_INVALID, CM_INFANTRY, CM_VEHICLE, CM_BOAT, CM_PLANE, CM_HELI, CM_STRUCT
};

class Sidebar {
public:
    Sidebar(Player *pl, Uint16 height, const char* theatre);
    ~Sidebar();

    bool getVisChanged();
    bool getVisible() {return visible;}
    void ToggleVisible();

    SDL_Surface* getTabImage() {return pc::imgcache->getImage(tab).image;}
    SDL_Rect* getTabLocation() {return &tablocation;}

    // Reloads the SDL_Surfaces
    void ReloadImages();

    SDL_Surface* getSidebarImage(SDL_Rect location);
    bool isOriginalType() {return isoriginaltype;}

    Uint8 getButton(Uint16 x, Uint16 y);
    void ClickButton(Uint8 index, char* unitname, createmode_t* createmode);
    void ResetButton();
    void ScrollSidebar(bool scrollup);
    void UpdateSidebar();

    void StartRadarAnim(Uint8 mode, bool* minienable);

    /// @TODO Continue almighty jihad against naked pointers
    Font* getFont() {return gamefnt.get();}

    Uint8 getSteps() const {return steps;}

    class SidebarError {};

    enum sidebarop {sbo_null = 0, sbo_build = 1, sbo_scroll = 2, sbo_unit = 4,
        sbo_structure = 8, sbo_up = 16, sbo_down = 32};

    struct SidebarGeometry {
        Uint16 bw,bh;
    };
    const SidebarGeometry& getGeom() {return geom;}
private:
    Sidebar() {};
    Sidebar(const Sidebar&) {};
    Sidebar& operator=(const Sidebar&) {return *this;}

    class RadarAnimEvent : public ActionEvent {
    public:
        RadarAnimEvent(Uint8 mode, bool* minienable, Uint32 radar);
        void run();
    private:
        Uint8 mode, frame, framend;
        bool* minienable;
        Uint32 radar;
    };

    friend class RadarAnimEvent;

    class SidebarButton {
    public:
        SidebarButton(Sint16 x, Sint16 y, const char* fname, Uint8 f,
                const char* theatre, Uint8 palnum);
        ~SidebarButton();
        void ChangeImage(const char* fname);
        void ChangeImage(const char* fname, Uint8 number);
        SDL_Surface* getSurface() const {
            return pic;
        }
        SDL_Rect getRect() const {
            return picloc;
        }
        Uint8 getFunction() const {
            return function;
        }
        SDL_Surface* Fallback(const char* fname);

        void ReloadImage();
    private:
        Uint32 picnum;
        SDL_Surface *pic;
        Uint8 function, palnum;
        const char* theatre;

        bool using_fallback;
        const char *fallbackfname;

        SDL_Rect picloc;
    };

    // True: DOS, False: GOLD
    bool isoriginaltype;
    // Palette offset for structures (Nod's buildings are red)
    Uint8 spalnum;

    void SetupButtons(Uint16 height);
    void ScrollBuildList(Uint8 dir, Uint8 type);
    void Build(Uint8 index, Uint8 type, char* unitname, createmode_t* createmode);
    void UpdateIcons();
    void UpdateAvailableLists();
    void DownButton(Uint8 index);
    void AddButton(Uint16 x, Uint16 y, const char* fname, Uint8 f, Uint8 pal);
    void DrawButton(Uint8 index);
    void DrawClock(Uint8 index, Uint8 imgnum);

    Uint32 radarlogo;
    SDL_Rect radarlocation;

    Uint32 tab;
    SDL_Rect tablocation;

    SDL_Surface* sbar;
    SDL_Rect sbarlocation;

    shared_ptr<Font> gamefnt;

    bool visible, vischanged;

    const char* theatre;

    Uint8 buttondown;
    bool bd;

    Uint8 buildbut;
    std::vector<shared_ptr<SidebarButton> > buttons;

    std::vector<char*> uniticons;
    std::vector<char*> structicons;

    const char* radarname;
    RadarAnimEvent* radaranim;
    bool radaranimating;

    Uint8 unitoff,structoff; // For scrolling

    Player* player;
    int scaleq;

    Uint8 steps;

    SidebarGeometry geom;

    bool greyFixed[256];
    void FixGrey(SDL_Surface* gr, Uint8 num);
};

#endif
