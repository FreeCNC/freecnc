#ifndef _GAME_MAP_H
#define _GAME_MAP_H

#include "../freecnc.h"
#include "../lib/inifile.h"

struct SDL_Surface;

class LoadingScreen;
class TemplateImage;
class Unit;
class SHPImage;

class CnCMap;

struct TerrainEntry
{
    Uint32 shpnum;
    Sint16 xoffset;
    Sint16 yoffset;
};

struct TileList
{
    Uint16 templateNum;
    Uint8 tileNum;
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
    Uint8 buildlevel;
};

struct ScrollVector {
    Sint8 x, y;
    Uint8 t;
};

struct ScrollBookmark {
    Uint16 x,y;
    Uint16 xtile,ytile;
};

struct ScrollData {
    Uint16 maxx, maxy;
    Uint16 curx, cury;
    Uint16 maxxtileoffs, maxytileoffs;
    Uint16 curxtileoffs, curytileoffs;
    Uint16 tilewidth;
};

struct MiniMapClipping {
    Uint16 x,y,w,h;
    Uint16 sidew, sideh;
    Uint8 tilew, tileh, pixsize;
};

struct TemplateTilePair {
    TemplateImage *theater; // Template for Theater
    Uint8 tile; //Tile number in this Theater
};

typedef std::map<std::string, TemplateImage*> TemplateCache;
typedef std::vector<TemplateTilePair* > TemplateTileCache;

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
    bool isBuildableAt(Uint16 pos, Unit* excpUn = 0) const;
    Uint16 getCost(Uint16 pos, Unit* excpUn = 0) const;
    Uint16 getWidth() const {return width;}
    Uint16 getHeight() const {return height;}
    Uint32 getSize() const {return width*height;}
    Uint32 translateToPos(Uint16 x, Uint16 y) const;
    void translateFromPos(Uint32 pos, Uint16 *x, Uint16 *y) const;
    enum ttype {t_land=0, t_water=1, t_road=2, t_rock=3, t_tree=4,
        t_water_blocked=5, t_other_nonpass=7};
    enum ScrollDirection {s_none = 0, s_up = 1, s_right = 2, s_down = 4, s_left = 8,
        s_upright = 3, s_downright = 6, s_downleft = 12, s_upleft = 9, s_all = 15};

    class LoadMapError {};

    // C/S: Not sure about this one
    Uint8 getGameMode() const {
        return gamemode;
    }

    // C/S: These functions are client only
    SDL_Surface *getMapTile( Uint32 pos ) {
        return tileimages[tilematrix[pos]];
    }
    SDL_Surface *getShadowTile(Uint8 shadownum) {
        if( shadownum >= numShadowImg ) {
            return NULL;
        }
        return shadowimages[shadownum];
    }
    bool getResource(Uint32 pos, Uint8* type, Uint8* amount) const {
        if (0 == type || 0 == amount) {
            return (0 != resourcematrix[pos]);
        }
        *type = resourcematrix[pos] & 0xFF;
        *amount = resourcematrix[pos] >> 8;
        return (0 != resourcematrix[pos]);
    }
    /// @returns the resource data in a form best understood by the
    /// imagecache/renderer
    Uint32 getResourceFrame(Uint32 pos) const {
        return ((resourcebases[resourcematrix[pos] & 0xFF]<<16) +
                (resourcematrix[pos]>>8));
    }
    /*
    Uint32 getTiberium(Uint32 pos) const {
        return (overlaymatrix[pos] & 0xF);
    }
    */
    Uint32 getSmudge(Uint32 pos) const {
        return ((overlaymatrix[pos] & 0xF0) << 12);
    }
    Uint32 setSmudge(Uint32 pos, Uint8 value) {
        /// clear the existing smudge bits first
        overlaymatrix[pos] &= ~(0xF0);
        return (overlaymatrix[pos] |= (value<<4));
    }
    Uint32 setTiberium(Uint32 pos, Uint8 value) {
        /// clear the existing tiberium bits first
        overlaymatrix[pos] &= ~0xF;
        return (overlaymatrix[pos] |= value);
    }
    Uint32 getOverlay(Uint32 pos);
    Uint32 getTerrain(Uint32 pos, Sint16* xoff, Sint16* yoff);
    Uint8 getTerrainType(Uint32 pos) const {
        return terraintypes[pos];
    }

    /// Reloads all the tiles SDL_Images
    void reloadTiles();

    Uint8 accScroll(Uint8 direction);
    Uint8 absScroll(Sint16 dx, Sint16 dy, Uint8 border);
    void doscroll();
    void setMaxScroll(Uint32 x, Uint32 y, Uint32 xtile, Uint32 ytile, Uint32 tilew);
    void setValidScroll();
    Uint32 getScrollPos() const {
        return scrollpos.cury*width+scrollpos.curx;
    }
    Uint16 getXScroll() const {
        return scrollpos.curx;
    }
    Uint16 getYScroll() const {
        return scrollpos.cury;
    }
    Uint16 getXTileScroll() const {
        return (Uint16)scrollpos.curxtileoffs;
    }
    Uint16 getYTileScroll() const {
        return (Uint16)scrollpos.curytileoffs;
    }

     SDL_Surface* getMiniMap(Uint8 pixside);
    void prepMiniClip(Uint16 sidew, Uint16 sideh) {
        miniclip.sidew = sidew;
        miniclip.sideh = sideh;
    }
    const MiniMapClipping& getMiniMapClipping() const {return miniclip;}

    bool toScroll() const {
        return toscroll;
    }
    void storeLocation(Uint8 loc);
    void restoreLocation(Uint8 loc);
    std::vector<Uint16> getWaypoints() {
        return waypoints;
    }
    SHPImage* getPips() {
        return pips;
    }
    Uint32 getPipsNum() const {
        return pipsnum;
    }
    SHPImage* getMoveFlash() {
        return moveflash;
    }
    Uint32 getMoveFlashNum() const {
        return flashnum;
    }
    // C/S: Client only?
    /// @TODO Explain what "X" means
    Uint16 getX() const {
        return x;
    }
    /// @TODO Explain what "X" means
    Uint16 getY() const {
        return y;
    }

    /// Checks the WW coord is valid
    bool validCoord(Uint16 tx, Uint16 ty) const {
        return (!(tx < x || ty < y || tx > x+width || ty > height+y));
    }
    /// Converts a WW coord into a more flexible coord
    Uint32 normaliseCoord(Uint32 linenum) const {
        Uint16 tx, ty;
        translateCoord(linenum, &tx, &ty);
        return normaliseCoord(tx, ty);
    }
    Uint32 normaliseCoord(Uint16 tx, Uint16 ty) const {
        return (ty-y)*width + tx - x;
    }
    void translateCoord(Uint32 linenum, Uint16* tx, Uint16* ty) const {
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
    static const Uint8 NUMMARKS=5;

    MissionData missionData;
    /// Load the ini part of the map
    void loadIni();
    /// The map section of the ini
    void simpleSections(shared_ptr<INIFile> inifile);

    /// The advanced section of the ini
    void advancedSections(shared_ptr<INIFile> inifile);

    /// Load the bin part of the map (TD)
    void loadBin();

    /// Load the overlay section of the map (TD)
    void loadOverlay(shared_ptr<INIFile> inifile);

    /// Extract RA map data
    void unMapPack(shared_ptr<INIFile> inifile);

    /// Extract RA overlay data
    void unOverlayPack(shared_ptr<INIFile> inifile);

    /// load the palette
    /// The only thing map specific about this function is the theatre (whose
    /// palette is then loaded into SHPBase).
    void loadPal(SDL_Color *palette);

    /// Parse the BIN part of the map (RA or TD)
    void parseBin(TileList *bindata);

    /// Parse the overlay part of the map (RA or TD)
    void parseOverlay(const Uint32& linenum, const std::string& name);

    /// load a specified tile
    SDL_Surface *loadTile(shared_ptr<INIFile> templini, Uint16 templ, Uint8 tile,
            Uint32* tiletype);

    /// width of map in tiles
    Uint16 width;
    /// height of map in tiles
    Uint16 height;
    /// x coordinate for the first tile
    /// @TODO Which means?
    Uint16 x;
    /// y coordinate for the first tile
    /// @TODO Which means?
    Uint16 y;

    /// Are we loading the map?
    bool loading;

    /// Have we loaded the map?
    bool loaded;

    ScrollData scrollpos;
    /* A array of tiles and a vector constining the images for the tiles */
    /// The matrix used to store terrain information.
    std::vector<Uint16> tilematrix;

    // Client only
    TemplateCache templateCache; //Holds cache of TemplateImage*s

    std::vector<SDL_Surface*> tileimages; //Holds the SDL_Surfaces of the TemplateImage
    TemplateTileCache templateTileCache; //Stores the TemplateImage* and Tile# of each SDL_Surface in tileimages

    Uint16 numShadowImg;
    std::vector<SDL_Surface*> shadowimages;

    // Both
    /// These come from the WAYPOINTS section of the inifile, and contain start
    /// locations for multiplayer maps.
    std::vector<Uint16> waypoints;

    std::vector<Uint32> overlaymatrix;

    std::vector<Uint16> resourcematrix;
    std::vector<Uint32> resourcebases;
    std::map<std::string, Uint8> resourcenames;

    std::vector<Uint8> terraintypes;

    std::map<Uint32, TerrainEntry> terrains;
    std::map<Uint32, Uint16> overlays;

    /// @TODO We get this from the game loader part, investigate if there's a better approach.
    Uint8 maptype;
    Uint8 gamemode; // 0 - single player, 1 - skirmish, 2 - multiplayer

    /// @TODO These need a better (client side only) home, (ui related)
    SDL_Surface *minimap, *oldmmap;
    MiniMapClipping miniclip;

    /// @TODO These need a better (client side only) home (ui related)
    ScrollVector scrollvec;
    bool toscroll;
    /// stores which directions can be scrolled
    Uint8 valscroll;
    /// stores certain map locations
    ScrollBookmark scrollbookmarks[NUMMARKS];
    Uint8 scrollstep, maxscroll, scrolltime;

    /// @TODO These need a better (client side only) home (ui/gfx related)
    Uint32 pipsnum;
    SHPImage* pips;
    Uint32 flashnum;
    SHPImage* moveflash;

    /// When converting WW style linenum values, do we use 64 or 128 as our
    /// modulus/divisor?
    bool translate_64;
};

#endif
