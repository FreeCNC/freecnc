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
    Sidebar(Player *pl, unsigned short height, const char* theatre);
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

    unsigned char getButton(unsigned short x, unsigned short y);
    void ClickButton(unsigned char index, char* unitname, createmode_t* createmode);
    void ResetButton();
    void ScrollSidebar(bool scrollup);
    void UpdateSidebar();

    void StartRadarAnim(unsigned char mode, bool* minienable);

    /// @TODO Continue almighty jihad against naked pointers
    Font* getFont() {return gamefnt.get();}

    unsigned char getSteps() const {return steps;}

    class SidebarError {};

    enum sidebarop {sbo_null = 0, sbo_build = 1, sbo_scroll = 2, sbo_unit = 4,
        sbo_structure = 8, sbo_up = 16, sbo_down = 32};

    struct SidebarGeometry {
        unsigned short bw,bh;
    };
    const SidebarGeometry& getGeom() {return geom;}
private:
    Sidebar() {};
    Sidebar(const Sidebar&) {};
    Sidebar& operator=(const Sidebar&) {return *this;}

    class RadarAnimEvent : public ActionEvent {
    public:
        RadarAnimEvent(unsigned char mode, bool* minienable, unsigned int radar);
        bool run();
    private:
        unsigned char mode, frame, framend;
        bool* minienable;
        unsigned int radar;
    };

    friend class RadarAnimEvent;

    class SidebarButton {
    public:
        SidebarButton(short x, short y, const char* fname, unsigned char f,
                const char* theatre, unsigned char palnum);
        ~SidebarButton();
        void ChangeImage(const char* fname);
        void ChangeImage(const char* fname, unsigned char number);
        SDL_Surface* getSurface() const {
            return pic;
        }
        SDL_Rect getRect() const {
            return picloc;
        }
        unsigned char getFunction() const {
            return function;
        }
        SDL_Surface* Fallback(const char* fname);

        void ReloadImage();
    private:
        unsigned int picnum;
        SDL_Surface *pic;
        unsigned char function, palnum;
        const char* theatre;

        bool using_fallback;
        const char *fallbackfname;

        SDL_Rect picloc;
    };

    // True: DOS, False: GOLD
    bool isoriginaltype;
    // Palette offset for structures (Nod's buildings are red)
    unsigned char spalnum;

    void SetupButtons(unsigned short height);
    void ScrollBuildList(unsigned char dir, unsigned char type);
    void Build(unsigned char index, unsigned char type, char* unitname, createmode_t* createmode);
    void UpdateIcons();
    void UpdateAvailableLists();
    void DownButton(unsigned char index);
    void AddButton(unsigned short x, unsigned short y, const char* fname, unsigned char f, unsigned char pal);
    void DrawButton(unsigned char index);
    void DrawClock(unsigned char index, unsigned char imgnum);

    unsigned int radarlogo;
    SDL_Rect radarlocation;

    unsigned int tab;
    SDL_Rect tablocation;

    SDL_Surface* sbar;
    SDL_Rect sbarlocation;

    shared_ptr<Font> gamefnt;

    bool visible, vischanged;

    const char* theatre;

    unsigned char buttondown;
    bool bd;

    unsigned char buildbut;
    std::vector<shared_ptr<SidebarButton> > buttons;

    std::vector<char*> uniticons;
    std::vector<char*> structicons;

    const char* radarname;
    shared_ptr<RadarAnimEvent> radaranim;
    bool radaranimating;

    unsigned char unitoff,structoff; // For scrolling

    Player* player;
    int scaleq;

    unsigned char steps;

    SidebarGeometry geom;

    bool greyFixed[256];
    void FixGrey(SDL_Surface* gr, unsigned char num);
};

#endif
