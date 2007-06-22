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
    Sidebar(Player* pl, int height, const char* theatre);
    ~Sidebar();

    bool get_vischanged();
    bool get_visible() {return visible;}
    void toggle_visible();

    SDL_Surface* get_tab_image() {return pc::imgcache->getImage(tab).image;}
    SDL_Rect* get_tablocation() {return &tablocation;}

    // Reloads the SDL_Surfaces
    void reload_images();

    SDL_Surface* get_sidebar_image(SDL_Rect location);

    unsigned char get_button(unsigned short x, unsigned short y);
    void click_button(unsigned char index, char* unitname, createmode_t* createmode);
    void reset_button();
    void scroll_sidebar(bool scrollup);
    void update_sidebar();

    void start_radar_anim(unsigned char mode, bool* minienable);

    /// @TODO Continue almighty jihad against naked pointers
    Font* get_font() {return gamefnt.get();}

    unsigned char get_steps() const {return steps;}

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

    enum SidebarType {INVALID, DOS, GOLD, REDALERT_NATO, REDALERT_USSR} sidebar_type;

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
                const char* theatre, unsigned char palnum, const SidebarType& st);
        ~SidebarButton();
        void change_image(const char* fname);
        void change_image(const char* fname, unsigned char number);
        SDL_Surface* getSurface() const {
            return pic;
        }
        SDL_Rect getRect() const {
            return picloc;
        }
        unsigned char getFunction() const {
            return function;
        }
        SDL_Surface* fallback(const char* fname);

        void reload_image();
    private:
        unsigned int picnum;
        SDL_Surface *pic;
        unsigned char function, palnum;
        const char* theatre;

        bool using_fallback;
        const char *fallbackfname;

        SDL_Rect picloc;
        const SidebarType& sidebar_type;
    };

    void setup_buttons();
    void scroll_build_list(unsigned char dir, unsigned char type);
    void build(unsigned char index, unsigned char type, char* unitname, createmode_t* createmode);
    void update_icons();
    void update_available_lists();
    void down_button(unsigned char index);
    void add_button(unsigned short x, unsigned short y, const char* fname, unsigned char f, unsigned char pal);
    void draw_button(unsigned char index);
    void draw_clock(unsigned char index, unsigned char imgnum);

    void load_images();
    void init_radar();
    void init_surface();
    void blit_static_images();

    int height;
    int unit_palette, structure_palette;

    unsigned int radarlogo;
    SDL_Rect radarlocation;

    unsigned int tab;
    SDL_Rect tablocation;

    SDL_Surface* sbar;
    SDL_Rect sbarlocation;

    shared_ptr<Font> gamefnt;

    bool visible, vischanged;
    bool invalidated;

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

    bool grey_fixed[256];
    void fix_grey(SDL_Surface* gr, unsigned char num);
};

#endif
