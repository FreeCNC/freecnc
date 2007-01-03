//
// This really ought to be in map.cpp
// 
#include <cctype>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "../lib/compression.h"
#include "../lib/inifile.h"
#include "../renderer/renderer_public.h"
#include "../legacyvfs/vfs_public.h"
#include "game.h"
#include "map.h"
#include "playerpool.h"
#include "unitandstructurepool.h"

using std::runtime_error;
using boost::lexical_cast;

/** Loads the maps ini file containing info on dimensions, units, trees
 * and so on.
 */

void CnCMap::loadIni()
{
    string map_filename(missionData.mapname);
    map_filename += ".INI";

    shared_ptr<INIFile> inifile;

    // Load the INIFile
    try {
        inifile = GetConfig(map_filename);
    } catch (runtime_error&) {
        game.log << "Map \"" << map_filename << "\" not found.  Check your installation." << endl;
        throw LoadMapError();
    }

    p::ppool = new PlayerPool(inifile, 0);

    if (inifile->readInt("basic", "newiniformat", 0) != 0) {
        game.log << "Red Alert maps not fully supported yet" << endl;
    }

    simpleSections(inifile);

    try {
        p::uspool = new UnitAndStructurePool();
    } catch (int) {
        throw LoadMapError();
    }

    p::ppool->setLPlayer(missionData.player);

    terraintypes.resize(width*height, 0);
    resourcematrix.resize(width*height, 0);

    advancedSections(inifile);

    if (maptype == GAME_RA)
        unMapPack(inifile);

    try {
        pips = new SHPImage("hpips.shp",mapscaleq);
    } catch(ImageNotFound&) {
        try {
            pips = new SHPImage("pips.shp", mapscaleq);
        } catch(ImageNotFound&) {
            game.log << "Unable to load the pips graphics!" << endl;
            throw LoadMapError();
        }
    }
    pipsnum = pc::imagepool->size()<<16;
    pc::imagepool->push_back(pips);

    if (maptype == GAME_RA) {
        try {
            char moveflsh[13] = "moveflsh.";
            strncat( moveflsh, missionData.theater, 3 );
            moveflash = new SHPImage(moveflsh,mapscaleq);
        } catch (ImageNotFound&) {
            game.log << "Unable to load the movement acknowledgement pulse graphic" << endl;
            throw LoadMapError();
        }
    } else {
        try {
            moveflash = new SHPImage("moveflsh.shp",mapscaleq);
        } catch (ImageNotFound&) {
            game.log << "Unable to load the movement acknowledgement pulse graphic" << endl;
            throw LoadMapError();
        }
    }
    flashnum = pc::imagepool->size()<<16;
    pc::imagepool->push_back(moveflash);
}


/** Function to load all vars in the simple sections of the inifile
 * @param pointer to the inifile
 */
void CnCMap::simpleSections(shared_ptr<INIFile> inifile) {
    const char* key;
    unsigned char iter = 0;

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

    while (strreads[iter] != 0) {
        char** variable;
        key = strreads[iter];
        variable = strvars[iter];
        *variable = inifile->readString("BASIC", key);
        if (0 == *variable) {
            game.log << "Error loading map: missing \"" << key << "\"" << endl;
            throw LoadMapError();
        }
        ++iter;
    }

    iter = 0;
    while (intreads[iter] != 0) {
        int temp;
        unsigned short* variable;
        key      = intreads[iter];
        variable = intvars[iter];

        temp = inifile->readInt("MAP", key);
        if (INIERROR == temp) {
            game.log << "Error loading map: unable to find \"" << key << "\"" << endl;
            throw LoadMapError();
        }

        *variable = temp;
        ++iter;
    }

    missionData.theater = inifile->readString("MAP", "THEATER");
    if (0 == missionData.theater) {
        game.log << "Error loading map: unable to find \"THEATER\"" << endl;
        throw LoadMapError();
    }

    missionData.buildlevel = inifile->readInt("BASIC", "BUILDLEVEL",1);
}

/** Function to load all the advanced sections in the inifile.
 * @param a pointer to the inifile
 */
void CnCMap::advancedSections(shared_ptr<INIFile> inifile) {
    unsigned int keynum;
    int i;
    //char *line;
    char shpname[128];
    char trigger[128];
    char action[128];
    char type[128];
    char owner[128];
    int facing, health, subpos;
    int linenum, smudgenum, tmpval;
    unsigned short tx, ty, xsize, ysize, tmp2;
    std::map<std::string, unsigned int> imagelist;
    std::map<std::string, unsigned int>::iterator imgpos;
    SHPImage *image;
    TerrainEntry tmpterrain;
    INIKey key;

    unsigned short xwalk, ywalk, ttype;

    try {
        for (keynum = 0; ; keynum++) {
            key = inifile->readKeyValue("WAYPOINTS", keynum);
            if (sscanf(key->first.c_str(), "%d", &tmpval) == 1) {
                if (tmpval == 26) { /* waypoint 26 is the startpos of the map */
                    tmp2 = (unsigned short)atoi(key->second.c_str());
                    //waypoints.push_back(tmp2);
                    translateCoord(tmp2, &tx, &ty);
                    scrollbookmarks[0].x = tx-x;
                    scrollbookmarks[0].y = ty-y;
                }
                if (tmpval < 8) {
                    tmp2 = (unsigned short)atoi(key->second.c_str());
                    waypoints.push_back(tmp2);
                }
            }
        }
    } catch(int) {}
    p::ppool->setWaypoints(waypoints);

    shared_ptr<INIFile> arts = GetConfig("art.ini");

    /* load the shadowimages */
    try {
        image = new SHPImage("SHADOW.SHP", mapscaleq);
        numShadowImg = image->getNumImg();
        shadowimages.resize(numShadowImg);
        for( i = 0; i < 48; i++ ) {
            image->getImageAsAlpha(i, &shadowimages[i]);
        }
        delete image;
    } catch(ImageNotFound&) {
        game.log << "Unable to load \"shadow.shp\"" << endl;
        numShadowImg = 0;
    }
    /* load the smudge marks and the tiberium to the imagepool */
    if (strncasecmp(missionData.theater, "INT", 3) != 0) {
        string sname;
        if (maptype == GAME_TD) {
            sname = "TI1";
        } else if (maptype == GAME_RA) {
            sname = "GOLD01";
        } else {
            game.log << "Unsuported maptype" << endl;
            throw LoadMapError();
        }

        resourcenames[sname] = 0;
        sname += "." + string(missionData.theater,3);
        try {
            image = new SHPImage(sname.c_str(), mapscaleq);
            resourcebases.push_back(pc::imagepool->size());
            pc::imagepool->push_back(image);
        } catch (ImageNotFound&) {
            game.log << "Could not load \"" << sname << "\"" << endl;
            throw LoadMapError();
        }
        // No craters or scorch marks for interior?
        for (i = 1; i <= 6; i++) {
            sprintf(shpname, "SC%d.", i);
            strncat(shpname, missionData.theater, 3);
            try {
                image = new SHPImage(shpname, mapscaleq);
            } catch (ImageNotFound&) {continue;}
            pc::imagepool->push_back(image);
        }
        for (i = 1; i <= 6; i++) {
            sprintf(shpname, "CR%d.", i);
            strncat(shpname, missionData.theater, 3);
            try {
                image = new SHPImage(shpname, mapscaleq);
            } catch (ImageNotFound&) {continue;}
            pc::imagepool->push_back(image);
        }
    }

    overlaymatrix.resize(width*height, 0);

    try {
        for( keynum = 0; ;keynum++ ) {
            bool bad = false;
            key = inifile->readKeyValue("TERRAIN", keynum);
            /* , is the char which separate terraintype from action. */

            if( sscanf(key->first.c_str(), "%d", &linenum) == 1 &&
                    sscanf(key->second.c_str(), "%[^,],", shpname) == 1 ) {
                /* Set the next entry in the terrain vector to the correct values.
                 * the map-array and shp files vill be set later */
                translateCoord(linenum, &tx, &ty);

                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }

                if( shpname[0] == 't' || shpname[0] == 'T' )
                    ttype = t_tree;
                else if( shpname[0] == 'r' || shpname[0] == 'R' )
                    ttype = t_rock;
                else
                    ttype = t_other_nonpass;

                /* calculate the new pos based on size and blocked */
                xsize = arts->readInt(shpname, "XSIZE",1);
                ysize = arts->readInt(shpname, "YSIZE",1);

                for( ywalk = 0; ywalk < ysize && ywalk + ty < height+y; ywalk++ ) {
                    for( xwalk = 0; xwalk < xsize && xwalk + tx < width + x; xwalk++ ) {
                        sprintf(type, "NOTBLOCKED%d", ywalk*xsize+xwalk);
                        if( arts->readInt(shpname, type) == INIERROR ) {
                            terraintypes[(ywalk+ty-y)*width+xwalk+tx-x] = ttype;
                        }
                    }
                }

                linenum = xsize*ysize;
                do {
                    if (linenum == 0) {
                        game.log << "BUG: Could not find an entry in art.ini for " << shpname << "" << endl;
                        bad = true;
                        break;
                    }
                    linenum--;
                    sprintf(type, "NOTBLOCKED%d", linenum);
                } while(arts->readInt(shpname, type) == INIERROR);
                if (bad)
                    continue;

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
                strcat(shpname, ".");
                strncat(shpname, missionData.theater, 3);

                /* search the map for the image */
                imgpos = imagelist.find(shpname);

                /* set up the overlay matrix and load some shps */
                if( imgpos != imagelist.end() ) {
                    /* this tile already has a number */
                    overlaymatrix[linenum] |= HAS_TERRAIN;
                    tmpterrain.shpnum = imgpos->second << 16;
                    terrains[linenum] = tmpterrain;
                } else {
                    /* a new tile */
                    imagelist[shpname] = pc::imagepool->size();
                    overlaymatrix[linenum] |= HAS_TERRAIN;
                    tmpterrain.shpnum = pc::imagepool->size()<<16;
                    terrains[linenum] = tmpterrain;
                    try {
                        image = new SHPImage(shpname, mapscaleq);
                    } catch (ImageNotFound&) {
                        game.log << "Could not load \"" << shpname << "\""  << endl;
                        throw LoadMapError();
                    }
                    pc::imagepool->push_back(image);
                }
            }
        }
    } catch(int) {}

    if (maptype == GAME_RA){
        unOverlayPack(inifile);
    } else {
        loadOverlay(inifile);
    }

    try {
        for( keynum = 0;;keynum++ ) {
            key = inifile->readKeyValue("SMUDGE", keynum);
            /* , is the char which separate terraintype from action. */
            if( sscanf(key->first.c_str(), "%d", &linenum) == 1 &&
                    sscanf(key->second.c_str(), "SC%d", &smudgenum) == 1 ) {
                translateCoord(linenum, &tx, &ty);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }
                linenum = (ty-y)*width + tx - x;
                overlaymatrix[linenum] |= (smudgenum<<4);
            } else if( sscanf(key->first.c_str(), "%d", &linenum) == 1 &&
                       sscanf(key->second.c_str(), "CR%d", &smudgenum) == 1 ) {
                //} else if( sscanf(line, "%d=CR%d", &linenum, &smudgenum) == 2 ) {
                translateCoord(linenum, &tx, &ty);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }

                linenum = (ty-y)*width + tx - x;
                overlaymatrix[linenum] |= ((smudgenum+6)<<4);
            }
        }
    } catch(int) {}

    p::uspool->preloadUnitAndStructures(missionData.buildlevel);
    p::uspool->generateProductionGroups();

    try {
        for( keynum = 0;;keynum++ ) {
            key = inifile->readKeyValue("STRUCTURES", keynum);
            /* , is the char which separate terraintype from action. */
            if( sscanf(key->first.c_str(), "%d", &tmpval) == 1 &&
                    sscanf(key->second.c_str(), "%[^,],%[^,],%d,%d,%d,%s", owner, type,
                           &health, &linenum, &facing, trigger ) == 6  ) {
                translateCoord(linenum, &tx, &ty);
                facing = min(31,facing>>3);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }
                linenum = (ty-y)*width + tx - x;
                p::uspool->createStructure(type, linenum, p::ppool->getPlayerNum(owner),
                        health, facing, false);
            }
        }
    } catch(int) {}

    try {
        for( keynum = 0;;keynum++ ) {
            key = inifile->readKeyValue("UNITS", keynum);
            /* , is the char which separate terraintype from action. */
            if( sscanf(key->first.c_str(), "%d", &tmpval) == 1 &&
                    sscanf(key->second.c_str(), "%[^,],%[^,],%d,%d,%d,%[^,],%s", owner, type,
                           &health, &linenum, &facing, action, trigger ) == 7  ) {
                translateCoord(linenum, &tx, &ty);
                facing = min(31,facing>>3);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }
                linenum = (ty-y)*width + tx - x;
                p::uspool->createUnit(type, linenum, 5, p::ppool->getPlayerNum(owner),
                        health, facing);
            }
        }
    } catch(int) {}

    /*infantry*/
    try {
        for( keynum = 0;;keynum++ ) {
            key = inifile->readKeyValue("INFANTRY", keynum);
            /* , is the char which separate terraintype from action. */
            if( sscanf(key->first.c_str(), "%d", &tmpval ) == 1  &&
                    sscanf(key->second.c_str(), "%[^,],%[^,],%d,%d,%d,%[^,],%d,%s", owner, type,
                           &health, &linenum, &subpos, action, &facing, trigger ) == 8  ) {
                translateCoord(linenum, &tx, &ty);
                facing = min(31,facing>>3);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }
                linenum = (ty-y)*width + tx - x;
                p::uspool->createUnit(type, linenum, subpos, p::ppool->getPlayerNum(owner),
                        health, facing);
            }
        }
    } catch(int) {}
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
    char *binname = new char[strlen(missionData.mapname)+5];

    TileList *mapdata;

    mapdata = new TileList[width*height];

    /* Calculate name of bin file ( mapname.bin ). */
    strcpy(binname, missionData.mapname);
    strcat(binname, ".BIN");

    /* get the offset and size of the binfile along with a pointer to it */
    //binfile = mixes->getOffsetAndSize(binname, &offset, &size);
    binfile = VFS_Open(binname);
    delete[] binname;

    if(binfile == NULL) {
        game.log << "Unable to locate BIN file!" << endl;
        throw LoadMapError();
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

void CnCMap::unMapPack(shared_ptr<INIFile> inifile)
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

                if( tileimg == NULL ) {
                    game.log << "Error loading tiles" << endl;
                    throw LoadMapError();
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

void CnCMap::loadOverlay(shared_ptr<INIFile> inifile)
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

void CnCMap::unOverlayPack(shared_ptr<INIFile> inifile)
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
            game.log << "Unable to load overlay \"" << shpname << "\" (or \"" << name << ".SHP\")" << endl;
            throw LoadMapError();
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
            game.log << "Resource hack for \"" << name << "\" failed." << endl;
            throw LoadMapError();
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
        game.log << "Unable to locate palette (\"" << palname << "\")." << endl;
        throw LoadMapError();
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
SDL_Surface *CnCMap::loadTile(shared_ptr<INIFile> templini, unsigned short templ, unsigned char tile, unsigned int* tiletype)
{
    TemplateCache::iterator ti;
    TemplateImage *theaterfile;

    SDL_Surface *retimage;

    char tilefilename[13];
    char tilenum[11];
    char *temname;

    /* The name of the file containing the template is something from
     * templates.ini . the three first
     * chars in the name of the theater eg. .DES .TEM .WIN */

    sprintf(tilefilename, "TEM%d", templ);
    sprintf(tilenum, "tiletype%d", tile);
    *tiletype = templini->readInt(tilefilename, tilenum, 0);

    temname = templini->readString(tilefilename, "NAME");

    if( temname == NULL ) {
        game.log << "Error in templates.ini! (can't find \"" << tilefilename << "\")"  << endl;
        strcpy(tilefilename, "CLEAR1");
    } else {
        strcpy(tilefilename, temname);
        delete[] temname;
    }

    strcat( tilefilename, "." );
    strncat( tilefilename, missionData.theater, 3 );

    // Check the templateCache
    ti = templateCache.find( tilefilename );
    // If we haven't preloaded this, lets do so now
    if (ti == templateCache.end()) {
        try {
            if( maptype == GAME_RA ) {
                theaterfile = new TemplateImage(tilefilename, mapscaleq, 1);
            } else {
                theaterfile = new TemplateImage(tilefilename, mapscaleq);
            }
        } catch(ImageNotFound&) {
            game.log << "Unable to locate template " << templ << ", " << tile << " (\"" << tilefilename << "\") in mix! using tile 0, 0 instead" << endl;
            if (templ == 0 && tile == 0) {
                game.log << "Unable to load tile 0,0.  Can't proceed" << endl;
                return NULL;
            }
            return loadTile( templini, 0, 0, tiletype );
        }

        // Store this TemplateImage for later use
        templateCache[tilefilename] = theaterfile;
    } else {
        theaterfile = ti->second;
    }

    //Now return this SDL_Surface
    retimage = theaterfile->getImage(tile);
    if( retimage == NULL ) {
        game.log << "Illegal template " << templ << ", " << tile << " (\"" << tilefilename << "\")! using tile 0, 0 instead" << endl;
        if( templ == 0 && tile == 0 )
            return NULL;
        return loadTile( templini, 0, 0, tiletype );
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

