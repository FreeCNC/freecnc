//
// This really ought to be in map.cpp
// 
#include <cctype>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include "../lib/compression.h"
#include "../lib/inifile.h"
#include "../renderer/renderer_public.h"
#include "../legacyvfs/vfs_public.h"
#include "game.h"
#include "map.h"
#include "playerpool.h"
#include "unitandstructurepool.h"

using boost::bind;
using boost::lexical_cast;
using boost::tokenizer;

namespace
{
    typedef tokenizer<boost::char_separator<char> > CharTokenizer;
    boost::char_separator<char> comma_sep(",");
}

/** Loads the maps ini file containing info on dimensions, units, trees
 * and so on.
 */

void CnCMap::loadIni()
{
    string map_filename(missionData.mapname);
    map_filename += ".INI";

    // Load the INIFile
    try {
        inifile = GetConfig(map_filename);
    } catch (std::runtime_error&) {
        throw MapLoadingError("Map loader: The map \"" + map_filename +
            "\" was not found.  Check your installation and files.ini");
    }

    p::ppool = new PlayerPool(inifile, 0);

    if (inifile->readInt("basic", "newiniformat", 0) != 0) {
        game.log << "Map loader: Warning: Red Alert maps are not fully supported yet" << endl;
    }

    simpleSections();

    p::uspool = new UnitAndStructurePool();

    p::ppool->setLPlayer(missionData.player);

    terraintypes.resize(width*height, 0);
    resourcematrix.resize(width*height, 0);

    advanced_sections();

    if (maptype == GAME_RA) {
        unMapPack();
    }

    try {
        pips = new SHPImage("hpips.shp",mapscaleq);
    } catch(ImageNotFound&) {
        try {
            pips = new SHPImage("pips.shp", mapscaleq);
        } catch(ImageNotFound&) {
            throw MapLoadingError("Map loader: Unable to load the pips graphics");
        }
    }
    pipsnum = pc::imagepool->size()<<16;
    pc::imagepool->push_back(pips);

    try {
        string flash_filename("moveflsh.");
        if (maptype == GAME_RA) {
            flash_filename += missionData.theater_prefix;
        } else {
            flash_filename += "shp";
        }
        moveflash = new SHPImage(flash_filename.c_str(), mapscaleq);
    } catch (ImageNotFound&) {
        throw MapLoadingError("Unable to load the movement acknowledgement pulse graphic");
    }

    flashnum = pc::imagepool->size()<<16;
    pc::imagepool->push_back(moveflash);

    inifile.reset();
}


/** Function to load all vars in the simple sections of the inifile
 * @param pointer to the inifile
 */
void CnCMap::simpleSections()
{
    // The strings in the basic section
    static const char* strreads[] = {
        "BRIEF", "ACTION", "PLAYER", "THEME", "WIN", "LOSE", 0
    };
    static char** strvars[] = {
        &missionData.brief, &missionData.action, &missionData.player,
        &missionData.theme, &missionData.winmov, &missionData.losemov,
    };
    static const char* intreads[] = {
        "HEIGHT", "WIDTH", "X", "Y", 0,
    };
    static unsigned short* intvars[]  = {&height, &width, &x, &y};

    int iter = 0;
    while (strreads[iter] != 0) {
        char** variable;
        const char* key = strreads[iter];
        variable = strvars[iter];
        *variable = inifile->readString("BASIC", key);
        if (0 == *variable) {
            throw MapLoadingError("Error loading map: missing \"" + string(key) + "\"");
        }
        ++iter;
    }

    iter = 0;
    while (intreads[iter] != 0) {
        int temp;
        unsigned short* variable;
        const char* key = intreads[iter];
        variable = intvars[iter];

        temp = inifile->readInt("MAP", key);
        if (INIERROR == temp) {
            string s("Map loader: Unable to find key \"");
            s += key;
            s += "\"";
            throw MapLoadingError(s);
        }

        *variable = temp;
        ++iter;
    }

    missionData.theater = inifile->readString("MAP", "THEATER");
    if (0 == missionData.theater) {
        throw MapLoadingError("Map loader: Unable to find \"THEATER\" section");
    }
    missionData.theater_prefix = string(missionData.theater, 3);

    missionData.buildlevel = inifile->readInt("BASIC", "BUILDLEVEL",1);
}

void CnCMap::load_terrain_detail(const char* prefix)
{
    static const string middles[] = {"1.", "2.", "3.", "4.", "5.", "6."};

    for (int i = 0; i < 6; ++i) {
        string shpname(prefix + middles[i] + missionData.theater_prefix);
        try {
            SHPImage* image = new SHPImage(shpname.c_str(), mapscaleq);
            pc::imagepool->push_back(image);
        } catch (ImageNotFound&) {
            continue;
        }
    }
}

void CnCMap::load_resources()
{
    string resource_name;
    if (maptype == GAME_TD) {
        resource_name = "TI1.";
    } else if (maptype == GAME_RA) {
        resource_name = "GOLD01.";
    } else {
        throw MapLoadingError("Map loader: Unsupported map type: " + string(missionData.theater));
    }

    resource_name += missionData.theater_prefix;
    try {
        SHPImage* image = new SHPImage(resource_name.c_str(), mapscaleq);
        resourcebases.push_back(pc::imagepool->size());
        pc::imagepool->push_back(image);
    } catch (ImageNotFound&) {
        throw MapLoadingError("Map loader: Could not load \"" + resource_name + "\"");
    }

    // No craters or scorch marks for interior?
    load_terrain_detail("SC");
    load_terrain_detail("CR");
}

void CnCMap::load_waypoint(const INISectionItem& key)
{
    int waypoint_number = lexical_cast<int>(key.first);

    unsigned short pos = (unsigned short)atoi(key.second.c_str());
    if (waypoint_number == 26) { // waypoint 26 is the startpos of the map
        unsigned short tx, ty;
        translateCoord(pos, &tx, &ty);
        scrollbookmarks[0].x = tx-x;
        scrollbookmarks[0].y = ty-y;
    }
    if (waypoint_number < 8) {
        waypoints.push_back(pos);
    }
}

void CnCMap::load_terrain(const INISectionItem& key)
{
    std::map<std::string, unsigned int> imagelist;

    shared_ptr<INIFile> arts = GetConfig("art.ini");

    bool bad = false;

    /* , is the char which separate terraintype from action. */

    int linenum = lexical_cast<int>(key.first);
    string::size_type comma_pos =  key.second.find(",");
    if (comma_pos == string::npos) {
        return;
    }
    string shpname(key.second, 0, comma_pos);

    /* Set the next entry in the terrain vector to the correct values.
     * the map-array and shp files vill be set later */
    unsigned short tx, ty;
    translateCoord(linenum, &tx, &ty);

    if( tx < x || ty < y || tx > x+width || ty > height+y ) {
        return;
    }

    unsigned char ttype;

    if (shpname[0] == 't' || shpname[0] == 'T') {
        ttype = t_tree;
    } else if (shpname[0] == 'r' || shpname[0] == 'R') {
        ttype = t_rock;
    } else {
        ttype = t_other_nonpass;
    }

    /* calculate the new pos based on size and blocked */
    unsigned int xsize = arts->readInt(shpname.c_str(), "XSIZE",1);
    unsigned int ysize = arts->readInt(shpname.c_str(), "YSIZE",1);

    for (unsigned int ywalk = 0; ywalk < ysize && ywalk + ty < height+y; ++ywalk) {
        for (unsigned int xwalk = 0; xwalk < xsize && xwalk + tx < width + x; ++xwalk ) {
            string type("NOTBLOCKED");
            type += lexical_cast<int>(ywalk*xsize+xwalk);
            int value = arts->readInt(shpname.c_str(), type.c_str());
            if (value == INIERROR) {
                terraintypes[(ywalk+ty-y)*width+xwalk+tx-x] = ttype;
            } else {
                // Now what?
            }
        }
    }

    linenum = xsize*ysize;

    string type;
    do {
        if (linenum == 0) {
            game.log << "BUG: Could not find an entry in art.ini for " << shpname << "" << endl;
            bad = true;
            break;
        }
        linenum--;
        type = "NOTBLOCKED" + lexical_cast<string>(linenum);
    } while(arts->readInt(shpname.c_str(), type.c_str()) == INIERROR);

    if (bad) {
        return;
    }

    TerrainEntry tmpterrain;
    tmpterrain.xoffset = -(linenum%ysize)*24;
    tmpterrain.yoffset = -(linenum/ysize)*24;

    tx += linenum%ysize;
    if( tx >= width+x ) {
        tmpterrain.xoffset += 1+tx-(width+x);
        tx = width+x-1;
    }
    ty += linenum/ysize;
    if( ty >= height+y ) {
        tmpterrain.yoffset += 1+ty-(height+y);
        ty = height+y-1;
    }

    linenum = normaliseCoord(tx, ty);
    shpname += "." + missionData.theater_prefix;

    std::map<std::string, unsigned int>::iterator imgpos = imagelist.find(shpname);

    // set up the overlay matrix and load some shps
    if (imgpos != imagelist.end()) {
        overlaymatrix[linenum] |= HAS_TERRAIN;
        tmpterrain.shpnum = imgpos->second << 16;
        terrains[linenum] = tmpterrain;
    } else {
        imagelist[shpname] = pc::imagepool->size();
        overlaymatrix[linenum] |= HAS_TERRAIN;
        tmpterrain.shpnum = pc::imagepool->size()<<16;
        terrains[linenum] = tmpterrain;
        try {
            SHPImage* image = new SHPImage(shpname.c_str(), mapscaleq);
            pc::imagepool->push_back(image);
        } catch (ImageNotFound&) {
            throw MapLoadingError("Map loader: Could not load \"" + string(shpname) + "\"");
        }
    }
}

void CnCMap::load_smudge_position(const INISectionItem& key)
{
    int smudgenum, linenum;
    unsigned short tx, ty;
    /* , is the char which separate terraintype from action. */
    if (sscanf(key.first.c_str(), "%d", &linenum) == 1 &&
            sscanf(key.second.c_str(), "SC%d", &smudgenum) == 1) {

        translateCoord(linenum, &tx, &ty);
        if (tx < x || ty < y || tx > x+width || ty > height+y) {
            return;
        }
        linenum = (ty-y)*width + tx - x;
        overlaymatrix[linenum] |= (smudgenum<<4);
    } else if (sscanf(key.first.c_str(), "%d", &linenum) == 1 &&
            sscanf(key.second.c_str(), "CR%d", &smudgenum) == 1) {
        translateCoord(linenum, &tx, &ty);
        if (tx < x || ty < y || tx > x+width || ty > height+y) {
            return;
        }

        linenum = (ty-y)*width + tx - x;
        overlaymatrix[linenum] |= ((smudgenum+6)<<4);
    }
}

void CnCMap::load_structure_position(const INISectionItem& key)
{
    // Currently unused, can't be duplicate anyway
    // int tmpval = lexical_cast<int>(key.first);
    CharTokenizer tok(key.second, comma_sep);
    if (std::distance(tok.begin(), tok.end()) != 6) {
        game.log << "Map loader: Malformed line in STRUCTURES section: " << key.second << endl;
        return;
    }
    CharTokenizer::iterator it = tok.begin();
    string owner = *it++;
    string type = *it++;
    int health  = lexical_cast<int>(*it++);
    int linenum = lexical_cast<int>(*it++);
    int facing  = lexical_cast<int>(*it++);
    string trigger = *it++;

    unsigned short tx, ty;
    translateCoord(linenum, &tx, &ty);
    facing = min(31,facing>>3);
    if( tx < x || ty < y || tx > x+width || ty > height+y ) {
        return;
    }
    linenum = (ty-y)*width + tx - x;
    p::uspool->createStructure(type.c_str(), linenum,
            p::ppool->getPlayerNum(owner.c_str()), health, facing, false);
}

void CnCMap::load_unit_position(const INISectionItem& key)
{
    // Currently unused, can't be duplicate anyway
    // int tmpval = lexical_cast<int>(key.first);
    CharTokenizer tok(key.second, comma_sep);
    if (std::distance(tok.begin(), tok.end()) != 7) {
        game.log << "Map loader: Malformed line in UNITS section: " << key.second << endl;
        return;
    }
    CharTokenizer::iterator it = tok.begin();
    string owner = *it++;
    string type = *it++;
    int health  = lexical_cast<int>(*it++);
    int linenum = lexical_cast<int>(*it++);
    int facing  = lexical_cast<int>(*it++);
    string action  = *it++;
    string trigger = *it++;

    unsigned short tx, ty;
    translateCoord(linenum, &tx, &ty);
    facing = min(31,facing>>3);
    if (tx < x || ty < y || tx > x+width || ty > height+y) {
        return;
    }
    linenum = (ty-y)*width + tx - x;
    p::uspool->createUnit(type.c_str(), linenum, 5, p::ppool->getPlayerNum(owner.c_str()),
            health, facing);
}

void CnCMap::load_infantry_position(const INISectionItem& key)
{
    // Currently unused, can't be duplicate anyway
    // int tmpval = lexical_cast<int>(key.first);
    CharTokenizer tok(key.second, comma_sep);
    if (std::distance(tok.begin(), tok.end()) != 8) {
        game.log << "Map loader: Malformed line in INFANTRY section: " << key.second << endl;
        return;
    }
    CharTokenizer::iterator it = tok.begin();
    string owner = *it++;
    string type = *it++;
    int health  = lexical_cast<int>(*it++);
    int linenum = lexical_cast<int>(*it++);
    int subpos  = lexical_cast<int>(*it++);
    string action  = *it++;
    int facing  = lexical_cast<int>(*it++);
    string trigger = *it++;

    unsigned short tx, ty;
    translateCoord(linenum, &tx, &ty);
    facing = min(31,facing>>3);
    if( tx < x || ty < y || tx > x+width || ty > height+y ) {
        return;
    }
    linenum = (ty-y)*width + tx - x;
    p::uspool->createUnit(type.c_str(), linenum, subpos, p::ppool->getPlayerNum(owner.c_str()),
            health, facing);
}

void CnCMap::advanced_sections()
{
    try {
        SHPImage image("SHADOW.SHP", mapscaleq);
        numShadowImg = image.getNumImg();
        shadowimages.resize(numShadowImg);
        for (int i = 0; i < 48; ++i) {
            image.getImageAsAlpha(i, &shadowimages[i]);
        }
    } catch(ImageNotFound&) {
        game.log << "Unable to load \"shadow.shp\"" << endl;
        numShadowImg = 0;
    }

    // load the smudge marks and the tiberium/ore to the imagepool
    if (missionData.theater_prefix != "INT") {
        load_resources();
    }

    overlaymatrix.resize(width*height, 0);

    if (maptype == GAME_RA) {
        unOverlayPack();
    } else {
        loadOverlay();
    }

    p::uspool->preloadUnitAndStructures(missionData.buildlevel);
    p::uspool->generateProductionGroups();

    static const char* sections[] = {"TERRAIN", "WAYPOINTS", "SMUDGE",
        "STRUCTURES", "UNITS", "INFANTRY"};

    typedef void (CnCMap::*section_parser_func)(const INISectionItem&);

    static const section_parser_func functions[] = {
        &CnCMap::load_terrain, &CnCMap::load_waypoint,
        &CnCMap::load_smudge_position, &CnCMap::load_structure_position,
        &CnCMap::load_unit_position, &CnCMap::load_infantry_position};

    static const int num_sections = sizeof(sections) / sizeof(const char*);

    int section_number;
    try {
        for (section_number = 0; section_number < num_sections; ++section_number) {
            INISection* data = inifile->section(sections[section_number]);
            if (data) {
                for_each(data->begin(), data->end(), bind(functions[section_number], this, _1));
            }
        }
    } catch (boost::bad_lexical_cast& e) {
        throw MapLoadingError("Unexpected value in the \"" + string(sections[section_number]) + "\" section");
    }

    p::ppool->setWaypoints(waypoints);
}



/////// Bin loading routines

struct tiledata
{
    unsigned int image;
    unsigned char type;
};

void CnCMap::loadBin()
{
    unsigned int index = 0;
    //    unsigned char templ, tile;
    int xtile, ytile;
    VFile *binfile;
    string bin_filename(missionData.mapname);

    TileList *mapdata;

    mapdata = new TileList[width*height];

    bin_filename += ".BIN";

    binfile = VFS_Open(bin_filename.c_str());

    if(binfile == NULL) {
        throw MapLoadingError("Map loader: Unable to open " + bin_filename);
    }

    /* Seek the beginning of the map.
     * It's at begining of bin + maxwidth * empty y cells + empty x cells
     * times 2 sinse each entry is 2 bytes
     */
    binfile->seekSet( (64*y + x) * 2 );

    for( ytile = 0; ytile < height; ytile++ ) {
        for( xtile = 0; xtile < width; xtile++ ) {
            unsigned short tmpread = 0;
            /* Read template and tile */
            mapdata[index].templateNum = 0;
            binfile->readByte((unsigned char *)&(tmpread), 1);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            mapdata[index].templateNum = SDL_Swap16(tmpread);
#else
            mapdata[index].templateNum = tmpread;
#endif
            binfile->readByte(&(mapdata[index].tileNum), 1);

            index++;
        }
        /* Skip til the end of the line and the onwards to the
         * beginning of usefull data on the next line 
         */
        binfile->seekCur( 2*(64-width) );
    }
    VFS_Close(binfile);
    parseBin(mapdata);
    delete[] mapdata;
}

void CnCMap::unMapPack()
{
    int tmpval;
    unsigned int curpos;
    int xtile, ytile;
    TileList *bindata;
    unsigned int keynum;
    INIKey key;
    unsigned char *mapdata1 = new unsigned char[49152]; // 48k
    unsigned char *mapdata2 = new unsigned char[49152];

    // read packed data into array
    mapdata1[0] = 0;
    try {
        for (keynum = 1;;++keynum) {
            key = inifile->readIndexedKeyValue("MAPPACK", keynum);
            strcat(((char*)mapdata1), key->second.c_str());
        }
    } catch(int) {}

    Compression::dec_base64(mapdata1,mapdata2,strlen(((char*)mapdata1)));

    /* decode the format80 coded data (6 chunks) */
    curpos = 0;
    for( tmpval = 0; tmpval < 6; tmpval++ ) {
        //printf("first vals in data is %x %x %x %x\n", mapdata2[curpos],
        //mapdata2[curpos+1], mapdata2[curpos+2], mapdata2[curpos+3]);
        if( Compression::decode80((unsigned char *)mapdata2+4+curpos, mapdata1+8192*tmpval) != 8192 ) {
            game.log << "A format80 chunk in the \"MapPack\" was of wrong size" << endl;
        }
        curpos = curpos + 4 + mapdata2[curpos] + (mapdata2[curpos+1]<<8) +
                 (mapdata2[curpos+2]<<16);
    }
    delete[] mapdata2;

    /* 128*128 16-bit template number followed by 128*128 8-bit tile numbers */
    bindata = new TileList[width*height];
    tmpval = y*128+x;
    curpos = 0;
    for( ytile = 0; ytile < height; ytile++ ) {
        for( xtile = 0; xtile < width; xtile++ ) {
            /* Read template and tile */
            bindata[curpos].templateNum = ((unsigned short *)mapdata1)[tmpval];
            bindata[curpos].tileNum = mapdata1[tmpval+128*128*2];
            curpos++;
            tmpval++;
            /*  printf("tile %d, %d\n", bindata[curpos-1].templateNum,
                 bindata[curpos-1].tileNum); */
        }
        /* Skip until the end of the line and the onwards to the
         * beginning of usefull data on the next line 
         */
        tmpval += (128-width);
    }
    delete[] mapdata1;
    parseBin(bindata);
    delete[] bindata;
}

void CnCMap::parseBin(TileList* bindata)
{
    unsigned int index;

    unsigned short templ;
    unsigned char tile;
    unsigned int tileidx;
    int xtile, ytile;
    shared_ptr<INIFile> templini;

    SDL_Surface *tileimg;
    SDL_Color palette[256];

    std::map<unsigned int, struct tiledata> tilelist;
    std::map<unsigned int, struct tiledata>::iterator imgpos;
    //     char tempc[sizeof(unsigned char)];

    struct tiledata tiledata;
    unsigned int tiletype;
    tilematrix.resize(width*height);

    loadPal(palette);
    SHPBase::setPalette(palette);
    SHPBase::calculatePalettes();

    /* Load the templates.ini */
    templini = GetConfig("templates.ini");

    index = 0;
    for( ytile = 0; ytile < height; ytile++ ) {
        for( xtile = 0; xtile < width; xtile++ ) {
            /* Read template and tile */
            templ = bindata[index].templateNum;
            tile  = bindata[index].tileNum;
            index++;
            /* Template 0xff is an empty tile */
            if(templ == ((maptype == GAME_RA)?0xffff:0xff)) {
                templ = 0;
                tile = 0;
            }

            /* Code sugested by Olaf van der Spek to cause all tiles in template
             * 0 and 2 to be used */
            if( templ == 0)
                tile = xtile&3 | (ytile&3 << 2);
            else if( templ == 2 )
                tile = xtile&1 | (ytile&1 << 1);

            imgpos = tilelist.find(templ<<8 | tile);

            /* set up the tile matrix and load some tiles */
            if( imgpos != tilelist.end() ) {
                /* this tile already has a number */
                tileidx = imgpos->second.image;
                tiletype = imgpos->second.type;
            } else {
                /* a new tile */
                tileimg = loadTile(templini, templ, tile, &tiletype);

                if (tileimg == NULL) {
                    throw MapLoadingError("Map loader: Error loading tiles");
                }

                tileidx = tileimages.size();
                tiledata.image = tileidx;
                tiledata.type = tiletype;
                tilelist[templ<<8 | tile] = tiledata;
                tileimages.push_back(tileimg);
            }

            // Set the tile in the tilematrix
            tilematrix[width*ytile+xtile] = tileidx;
            if( terraintypes[width*ytile+xtile] == 0 )
                terraintypes[width*ytile+xtile] = tiletype;


        }
    }
}

/////// Overlay loading routines

void CnCMap::loadOverlay()
{
    INIKey key;
    unsigned int linenum;
    unsigned short tx, ty;
    try {
        for (unsigned int keynum = 0;;keynum++) {
            key = inifile->readKeyValue("OVERLAY", keynum);
            if (sscanf(key->first.c_str(), "%u", &linenum) == 1) {
                translateCoord(linenum, &tx, &ty);
                if (!validCoord(tx, ty))
                    continue;
                linenum = normaliseCoord(tx, ty);

                parseOverlay(linenum, key->second);
            }
        }
    } catch(int) {}
}

const char* RAOverlayNames[] = { "SBAG", "CYCL", "BRIK", "FENC", "WOOD",
    "GOLD01", "GOLD02", "GOLD03", "GOLD04", "GEM01", "GEM02", "GEM03",
    "GEM04", "V12", "V13", "V14", "V15", "V16", "V17", "V18", "FPLS",
    "WCRATE", "SCRATE", "FENC", "SBAG" };

void CnCMap::unOverlayPack()
{
    unsigned int curpos, tilepos;
    unsigned char xtile, ytile;
    unsigned int keynum;
    INIKey key;
    unsigned char mapdata[16384]; // 16k
    unsigned char temp[16384];

    // read packed data into array
    mapdata[0] = 0;
    try {
        for (keynum = 1;;++keynum) {
            key = inifile->readIndexedKeyValue("OVERLAYPACK", keynum);
            strcat(((char*)mapdata), key->second.c_str());
        }
    } catch(int) {}

    Compression::dec_base64(mapdata, temp, strlen(((char*)mapdata)));

    /* decode the format80 coded data (2 chunks) */
    curpos = 0;
    for (int tmpval = 0; tmpval < 2; tmpval++) {
        if (Compression::decode80((unsigned char *)temp+4+curpos, mapdata+8192*tmpval) != 8192) {
            game.log << "A format80 chunk in the \"OverlayPack\" was of wrong size" << endl;
        }
        curpos = curpos + 4 + temp[curpos] + (temp[curpos+1]<<8) +
            (temp[curpos+2]<<16);
    }

    for (ytile = y; ytile <= y+height; ++ytile){
        for (xtile = x; xtile <= x+width; ++xtile){
            curpos = xtile+ytile*128;
            tilepos = xtile-x+(ytile-y)*width;
            if (mapdata[curpos] == 0xff) // No overlay
                continue;
            if (mapdata[curpos] > 0x17) // Unknown overlay type
                continue;
            parseOverlay(tilepos, RAOverlayNames[mapdata[curpos]]);
        }
    }
}


void CnCMap::parseOverlay(const unsigned int& linenum, const string& name)
{
    unsigned char type, frame;
    unsigned short res;

    if (name == "BRIK" || name == "SBAG" || name == "FENC" || name == "WOOD" || name == "CYCL" || name == "BARB") {
        // Walls are structures.
        p::uspool->createStructure(name.c_str(), linenum, p::ppool->getPlayerNum("NEUTRAL"), 256, 0, false);
        return;
    }

    string shpname;
    shpname = name + '.' + string(missionData.theater, 3);
    try {
        /* Remember: imagecache's indexing format is different
         * (imagepool index << 16) | frame */
        frame = pc::imgcache->loadImage(shpname.c_str()) >> 16;
    } catch(ImageNotFound&) {
        shpname = name + ".SHP";
        try {
            frame = pc::imgcache->loadImage(shpname.c_str()) >> 16;
        } catch (ImageNotFound&) {
            throw MapLoadingError("Unable to load overlay \"" + shpname + "\" or \"" + name + ".SHP\"");
        }
    }

    /// @TODO Generic resources?
    if (strncasecmp(name.c_str(),"TI",2) == 0 ||
            strncasecmp(name.c_str(),"GOLD",4) == 0 ||
            strncasecmp(name.c_str(),"GEM",3) == 0) {
        unsigned int i = 0;
        /* This is a hack to seed the map with semi-reasonable amounts of
         * resource growth.  This will hopefully become less ugly after the code
         * to manage resource growth has been written. */
        if (sscanf(name.c_str(), "TI%u", &i) == 0) {
            i = atoi(name.c_str() + (name.length() - 2));
            /* An even worse hack: number of frames in gems is less than the
             * number of different types of gem. */
            if ('E' == name[1]) i = 3;
        }
        if (0 == i) {
            throw MapLoadingError("Resource hack for \"" + string(name) + "\" failed.");
        }
        map<string, unsigned char>::iterator t = resourcenames.find(name);
        if (resourcenames.end() == t) {
            type = resourcebases.size();
            /* Encode the type and amount data into the resource matrix's new
             * cell. */
            res = type | ((i-1) << 8);
            resourcenames[name] = type;
            resourcebases.push_back(frame);
        } else {
            res = t->second | ((i-1) << 8);
        }
        resourcematrix[linenum] = res;
    } else {
        overlaymatrix[linenum] |= HAS_OVERLAY;
        overlays[linenum] = frame;

        if (toupper(name[0]) == 'T')
            terraintypes[linenum] = t_tree;
        else if (toupper(name[0]) == 'R')
            terraintypes[linenum] = t_rock;
        else
            terraintypes[linenum] = t_other_nonpass;
    }
}

/** Load a palette
 * @param palette array of SDL_Colour into which palette is loaded.
 */

void CnCMap::loadPal(SDL_Color *palette)
{
    VFile *palfile;
    int i;
    string palname = missionData.theater;
    if (palname.length() > 8) {
        palname.insert(8, ".PAL");
    } else {
        palname += ".PAL";
    }
    /* Seek the palette file in the mix */
    palfile = VFS_Open(palname.c_str());

    if (palfile == NULL) {
        throw MapLoadingError("Unable to locate palette (\"" + palname + "\").");
    }

    /* Load the palette */
    for (i = 0; i < 256; i++) {
        palfile->readByte(&palette[i].r, 1);
        palfile->readByte(&palette[i].g, 1);
        palfile->readByte(&palette[i].b, 1);
        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
    }
    VFS_Close(palfile);
}

/** load a tile from the mixfile.
 * @param the mixfiles.
 * @param the template inifile.
 * @param the template number.
 * @param the tilenumber.
 * @returns a SDL_Surface containing the tile.
 */
SDL_Surface *CnCMap::loadTile(shared_ptr<INIFile> templini, unsigned int templ, unsigned int tile, unsigned int* tiletype)
{
    TemplateImage *theaterfile;

    SDL_Surface *retimage;

    string tile_filename("TEM");
    string tile_number("tiletype");

    /* The name of the file containing the template is something from
     * templates.ini . the three first
     * chars in the name of the theater eg. .DES .TEM .WIN */

    tile_filename += lexical_cast<string>(templ);
    tile_number += lexical_cast<string>(tile);

    *tiletype = templini->readInt(tile_filename.c_str(), tile_number.c_str(), 0);

    // TODO: stringfy this after new INIFile
    char* temp_name = templini->readString(tile_filename.c_str(), "NAME");

    if (temp_name == NULL) {
        game.log << "Map loader: Error in templates.ini: can't find \"" << tile_filename << "\""  << endl;
        tile_filename = "CLEAR1";
    } else {
        tile_filename = temp_name;
        delete[] temp_name;
    }

    tile_filename += "." + missionData.theater_prefix;

    // Check the templateCache
    TemplateCache::iterator ti = templateCache.find(tile_filename);
    // If we haven't preloaded this, lets do so now
    if (ti == templateCache.end()) {
        try {
            if( maptype == GAME_RA ) {
                theaterfile = new TemplateImage(tile_filename.c_str(), mapscaleq, 1);
            } else {
                theaterfile = new TemplateImage(tile_filename.c_str(), mapscaleq);
            }
        } catch(ImageNotFound&) {
            game.log << "Unable to locate template " << templ << ", " << tile << " (\"" << tile_filename << "\") in mix! using tile 0, 0 instead" << endl;
            if (templ == 0 && tile == 0) {
                throw MapLoadingError("Map loader: Unable to load tile 0,0.  Can't proceed");
            }
            return loadTile(templini, 0, 0, tiletype);
        }

        // Store this TemplateImage for later use
        templateCache[tile_filename] = theaterfile;
    } else {
        theaterfile = ti->second;
    }

    //Now return this SDL_Surface
    retimage = theaterfile->getImage(tile);
    if (retimage == NULL) {
        game.log << "Illegal template " << templ << ", " << tile << " (\"" << tile_filename << "\")! using tile 0, 0 instead" << endl;
        assert(templ != 0 && tile != 0);
        return loadTile(templini, 0, 0, tiletype);
    }

    // Save a cache of this TemplateImage & Tile, so we can reload the SDL_Surface later
    TemplateTilePair* pair = new TemplateTilePair;
    pair->theater = theaterfile;
    pair->tile = tile;

    templateTileCache.push_back(pair);

    return retimage;
}

/** Reloads all the tile's SDL_Image
 */
void CnCMap::reloadTiles() {

    SDL_Surface *image;

    /* Free the old surfaces */
    for(std::vector<SDL_Surface*>::size_type i = 0; i < tileimages.size(); i++ )
        SDL_FreeSurface(tileimages[i]);

    tileimages.clear();

    for (TemplateTileCache::iterator i = templateTileCache.begin(); i != templateTileCache.end(); ++i) {
        TemplateTilePair* pair = *i;
        image = pair->theater->getImage(pair->tile);
        tileimages.push_back(image);
    }
}

