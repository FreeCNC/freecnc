#ifndef _GAME_MAP_H
#define _GAME_MAP_H

#include "../basictypes.h"
#include "../lib/inifile.h"

struct SDL_Surface;

class LoadingScreen;
class TemplateImage;
class Unit;
class SHPImage;

class CnCMap;

struct SDL_Color;

struct TerrainEntry
{
    unsigned int shpnum;
    short xoffset;
    short yoffset;
};

struct TileList
{
    unsigned short templateNum;
    unsigned char tileNum;
};

struct MissionData {
    MissionData() {
        theater = brief = action = player = theme = winmov = losemov = NULL;
        mapname = NULL;
    }

    ~MissionData() {
        delete[] theater;
        delete[] brief;
        delete[] action;
        delete[] player;
        delete[] theme;
        delete[] winmov;
        delete[] losemov;
    }
    /// the theater of the map (eg. winter)
    char* theater;
    /// the briefing movie
    char* brief;
    /// the actionmovie
    char* action;
    /// the players side (goodguy, badguy etc.)
    char* player;
    /// specific music to play for this mission
    char* theme;
    /// movie played after completed mission
    char* winmov;
    /// movie played after failed mission
    char* losemov;

    /// the name of the map
    const char* mapname;

    /// used to determine what units/structures can be built
    unsigned char buildlevel;

    string theater_prefix;
};

struct ScrollVector {
    char x, y;
    unsigned char t;
};

struct ScrollBookmark {
    unsigned short x,y;
    unsigned short xtile,ytile;
};

struct ScrollData {
    unsigned short maxx, maxy;
    unsigned short curx, cury;
    unsigned short maxxtileoffs, maxytileoffs;
    unsigned short curxtileoffs, curytileoffs;
    unsigned short tilewidth;
};

struct MiniMapClipping {
    unsigned short x,y,w,h;
    unsigned short sidew, sideh;
    unsigned char tilew, tileh, pixsize;
};

struct TemplateTilePair {
    TemplateImage *theater; // Template for Theater
    unsigned char tile; //Tile number in this Theater
};

typedef std::map<std::string, TemplateImage*> TemplateCache;
typedef std::vector<TemplateTilePair* > TemplateTileCache;

struct MapLoadingError : std::runtime_error
{
    MapLoadingError(const string& message) : std::runtime_error(message) {}
};

class CnCMap {
public:
    CnCMap();
    ~CnCMap();

    // Comments with "C/S:" at the start are to do with the client/server split.
    // C/S: Members used in both client and server
    void loadMap(const char* mapname, LoadingScreen* lscreen);

    const MissionData& getMissionData() const {
        return missionData;
    }

    bool isLoading() const {
        return loading;
    }
    bool isBuildableAt(unsigned short pos, Unit* excpUn = 0) const;
    unsigned short getCost(unsigned short pos, Unit* excpUn = 0) const;
    unsigned short getWidth() const {return width;}
    unsigned short getHeight() const {return height;}
    unsigned int getSize() const {return width*height;}
    unsigned int translateToPos(unsigned short x, unsigned short y) const;
    void translateFromPos(unsigned int pos, unsigned short *x, unsigned short *y) const;
    enum ttype {t_land=0, t_water=1, t_road=2, t_rock=3, t_tree=4,
        t_water_blocked=5, t_other_nonpass=7};
    enum ScrollDirection {s_none = 0, s_up = 1, s_right = 2, s_down = 4, s_left = 8,
        s_upright = 3, s_downright = 6, s_downleft = 12, s_upleft = 9, s_all = 15};

    // C/S: Not sure about this one
    unsigned char getGameMode() const {
        return 0;
    }

    // C/S: These functions are client only
    SDL_Surface *getMapTile( unsigned int pos ) {
        return tileimages[tilematrix[pos]];
    }
    SDL_Surface *getShadowTile(unsigned char shadownum) {
        if( shadownum >= numShadowImg ) {
            return NULL;
        }
        return shadowimages[shadownum];
    }
    bool getResource(unsigned int pos, unsigned char* type, unsigned char* amount) const {
        if (0 == type || 0 == amount) {
            return (0 != resourcematrix[pos]);
        }
        *type = resourcematrix[pos] & 0xFF;
        *amount = resourcematrix[pos] >> 8;
        return (0 != resourcematrix[pos]);
    }
    /// @returns the resource data in a form best understood by the
    /// imagecache/renderer
    unsigned int getResourceFrame(unsigned int pos) const {
        return ((resourcebases[resourcematrix[pos] & 0xFF]<<16) +
                (resourcematrix[pos]>>8));
    }
    /*
    unsigned int getTiberium(unsigned int pos) const {
        return (overlaymatrix[pos] & 0xF);
    }
    */
    unsigned int getSmudge(unsigned int pos) const {
        return ((overlaymatrix[pos] & 0xF0) << 12);
    }
    unsigned int setSmudge(unsigned int pos, unsigned char value) {
        /// clear the existing smudge bits first
        overlaymatrix[pos] &= ~(0xF0);
        return (overlaymatrix[pos] |= (value<<4));
    }
    unsigned int setTiberium(unsigned int pos, unsigned char value) {
        /// clear the existing tiberium bits first
        overlaymatrix[pos] &= ~0xF;
        return (overlaymatrix[pos] |= value);
    }
    unsigned int getOverlay(unsigned int pos);
    unsigned int getTerrain(unsigned int pos, short* xoff, short* yoff);
    unsigned char getTerrainType(unsigned int pos) const {
        return terraintypes[pos];
    }

    /// Reloads all the tiles SDL_Images
    void reloadTiles();

    unsigned char accScroll(unsigned char direction);
    unsigned char absScroll(short dx, short dy, unsigned char border);
    void doscroll();
    void setMaxScroll(unsigned int x, unsigned int y, unsigned int xtile, unsigned int ytile, unsigned int tilew);
    void setValidScroll();
    unsigned int getScrollPos() const {
        return scrollpos.cury*width+scrollpos.curx;
    }
    unsigned short getXScroll() const {
        return scrollpos.curx;
    }
    unsigned short getYScroll() const {
        return scrollpos.cury;
    }
    unsigned short getXTileScroll() const {
        return (unsigned short)scrollpos.curxtileoffs;
    }
    unsigned short getYTileScroll() const {
        return (unsigned short)scrollpos.curytileoffs;
    }

     SDL_Surface* getMiniMap(unsigned char pixside);
    void prepMiniClip(unsigned short sidew, unsigned short sideh) {
        miniclip.sidew = sidew;
        miniclip.sideh = sideh;
    }
    const MiniMapClipping& getMiniMapClipping() const {return miniclip;}

    bool toScroll() const {
        return toscroll;
    }
    void storeLocation(unsigned char loc);
    void restoreLocation(unsigned char loc);
    std::vector<unsigned short> getWaypoints() {
        return waypoints;
    }
    SHPImage* getPips() {
        return pips;
    }
    unsigned int getPipsNum() const {
        return pipsnum;
    }
    SHPImage* getMoveFlash() {
        return moveflash;
    }
    unsigned int getMoveFlashNum() const {
        return flashnum;
    }
    // C/S: Client only?
    /// @TODO Explain what "X" means
    unsigned short getX() const {
        return x;
    }
    /// @TODO Explain what "X" means
    unsigned short getY() const {
        return y;
    }

    /// Checks the WW coord is valid
    bool validCoord(unsigned short tx, unsigned short ty) const {
        return (!(tx < x || ty < y || tx > x+width || ty > height+y));
    }
    /// Converts a WW coord into a more flexible coord
    unsigned int normaliseCoord(unsigned int linenum) const {
        unsigned short tx, ty;
        translateCoord(linenum, &tx, &ty);
        return normaliseCoord(tx, ty);
    }
    unsigned int normaliseCoord(unsigned short tx, unsigned short ty) const {
        return (ty-y)*width + tx - x;
    }
    void translateCoord(unsigned int linenum, unsigned short* tx, unsigned short* ty) const {
        if (translate_64) {
            *tx = linenum%64;
            *ty = linenum/64;
        } else {
            *tx = linenum%128;
            *ty = linenum/128;
        }
    }
private:
    enum {HAS_OVERLAY=0x100, HAS_TERRAIN=0x200};
    static const unsigned char NUMMARKS=5;

    MissionData missionData;
    /// Load the ini part of the map
    void loadIni();

    /// The map section of the ini
    void simpleSections();

    /// The advanced section of the ini
    void advanced_sections();

    /// Load the bin part of the map (TD)
    void loadBin();

    void load_terrain_detail(const char* prefix);

    void load_resources();

    void load_waypoint(const INISectionItem&);

    void load_terrain(const INISectionItem&);

    void load_smudge_position(const INISectionItem&);

    void load_structure_position(const INISectionItem&);

    void load_unit_position(const INISectionItem&);

    void load_infantry_position(const INISectionItem&);

    /// Load the overlay section of the map (TD)
    void loadOverlay();

    /// Extract RA map data
    void unMapPack();

    /// Extract RA overlay data
    void unOverlayPack();

    /// load the palette
    /// The only thing map specific about this function is the theatre (whose
    /// palette is then loaded into SHPBase).
    void loadPal(SDL_Color *palette);

    /// Parse the BIN part of the map (RA or TD)
    void parseBin(TileList *bindata);

    /// Parse the overlay part of the map (RA or TD)
    void parseOverlay(const unsigned int& linenum, const std::string& name);

    /// load a specified tile
    SDL_Surface *loadTile(shared_ptr<INIFile> templini, unsigned int templ,
        unsigned int tile, unsigned int* tiletype);

    /// width of map in tiles
    unsigned short width;
    /// height of map in tiles
    unsigned short height;
    /// x coordinate for the first tile
    /// @TODO Which means?
    unsigned short x;
    /// y coordinate for the first tile
    /// @TODO Which means?
    unsigned short y;

    /// Are we loading the map?
    bool loading;

    /// Have we loaded the map?
    bool loaded;

    ScrollData scrollpos;
    /* A array of tiles and a vector constining the images for the tiles */
    /// The matrix used to store terrain information.
    std::vector<unsigned short> tilematrix;

    // Client only
    TemplateCache templateCache; //Holds cache of TemplateImage*s

    std::vector<SDL_Surface*> tileimages; //Holds the SDL_Surfaces of the TemplateImage
    TemplateTileCache templateTileCache; //Stores the TemplateImage* and Tile# of each SDL_Surface in tileimages

    unsigned short numShadowImg;
    std::vector<SDL_Surface*> shadowimages;

    // Both
    /// These come from the WAYPOINTS section of the inifile, and contain start
    /// locations for multiplayer maps.
    std::vector<unsigned short> waypoints;

    std::vector<unsigned int> overlaymatrix;

    std::vector<unsigned short> resourcematrix;
    std::vector<unsigned int> resourcebases;
    std::map<std::string, unsigned char> resourcenames;

    std::vector<unsigned char> terraintypes;

    std::map<unsigned int, TerrainEntry> terrains;
    std::map<unsigned int, unsigned short> overlays;

    /// @TODO We get this from the game loader part, investigate if there's a better approach.
    unsigned char maptype;

    /// @TODO These need a better (client side only) home, (ui related)
    SDL_Surface *minimap, *oldmmap;
    MiniMapClipping miniclip;

    /// @TODO These need a better (client side only) home (ui related)
    ScrollVector scrollvec;
    bool toscroll;
    /// stores which directions can be scrolled
    unsigned char valscroll;
    /// stores certain map locations
    ScrollBookmark scrollbookmarks[NUMMARKS];
    unsigned char scrollstep, maxscroll, scrolltime;

    /// @TODO These need a better (client side only) home (ui/gfx related)
    unsigned int pipsnum;
    SHPImage* pips;
    unsigned int flashnum;
    SHPImage* moveflash;

    /// When converting WW style linenum values, do we use 64 or 128 as our
    /// modulus/divisor?
    bool translate_64;

    // Only valid whilst the map is loading
    shared_ptr<INIFile> inifile;
};

#endif
