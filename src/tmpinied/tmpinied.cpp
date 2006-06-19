#include <cstdlib>
#include <string>

#include "SDL.h"

#include "../vfs/vfs_public.h"

#include "font.h"
#include "logger.h"
#include "shpimage.h"
#include "inifile.h"
#include "common.h"
#include "tmpgfx.h"
#include "templatedata.h"

using std::string;

Logger* logger;

#define TEMPLATES 216

void loadPal(char *palname);


TemplateImage *TemplateData::getImage()
{
    char fname[16];
    TemplateImage *img;
    try {
        sprintf(fname, "%s.sno", name);
        img = new TemplateImage(fname,-1);
        loadPal("snow.pal");
        printf("%s\n",fname);
    } catch(ImageNotFound&) {
        try {
            sprintf(fname, "%s.des", name);
            img = new TemplateImage(fname,-1);
            loadPal("desert.pal");
        } catch(ImageNotFound&) {
            try {
                sprintf(fname, "%s.win", name);
                img = new TemplateImage(fname,-1);
                loadPal("winter.pal");
            } catch(ImageNotFound&) {
                try {
                    sprintf(fname,"%s.tem", name);
                    img = new TemplateImage(fname,-1);
                    loadPal("temperat.pal");
                } catch(ImageNotFound&) {
                    try {
                        sprintf(fname,"%s.int", name);
                        img = new TemplateImage(fname,-1);
                        loadPal("interior.pal");
                    } catch(ImageNotFound&) {
                        fprintf(stderr, "Unable to load the template %s\n", name);
                        exit(1);
                    }
                }
            }
        }
    }
    return img;
}

TemplateData::TemplateData(INIFile *tmpini, Uint32 tmpnum)
{
    Uint32 i, type;
    //   char tmpnumname[16];
    char fname[16];
    TemplateImage *img;

    //this->mixes = mixes;

    sprintf(tmpnumname, "TEM%d", tmpnum);
    name = tmpini->readString(tmpnumname, "Name");
    if( name == NULL ) {
        fprintf(stderr, "Error in templates.ini (wrong name) (TEM%i)\n",tmpnum);
        exit(1);
    }
    img = getImage();

    numtiles = img->getNumTiles();
    width = tmpini->readInt(tmpnumname, "width");
    height = tmpini->readInt(tmpnumname, "height");
    if( width == INIERROR || height == INIERROR ) {
        width = numtiles;
        height = 1;
    }

    types = new Uint8[numtiles];
    for( i = 0; i < numtiles; i++ ) {
        sprintf(fname, "tiletype%d", i);
        type = tmpini->readInt(tmpnumname, fname);
        if( type == INIERROR )
            type = 0;
        types[i] = type;
    }
    delete img;
}

TemplateData::~TemplateData()
{
    free(name);
    delete[] types;
}

void TemplateData::increaseWidth()
{
    while( width < numtiles ) {
        width++;
        if( numtiles%width == 0 ) {
            break;
        }
    }
    height = numtiles/width;
}

void TemplateData::decreaseWidth()
{
    while( width > 1 ) {
        width--;
        if( numtiles%width == 0 ) {
            break;
        }
    }
    height = numtiles/width;
}

class TmpIniEd
{
public:
    TmpIniEd();
    ~TmpIniEd();
    void run();
private:
    //MIXFiles *mixes;
    Font *fnt;
    TmpGFX *gfx;
    TemplateData **tmps;
    Uint32 currentTmp;
    //   void loadPal(char *palname);
    TemplateImage *trans;
    void saveIni();
};

TmpIniEd::TmpIniEd()
{
    Uint32 i;
    INIFile *templini;
    gfx = new TmpGFX();
    fnt = new Font("6point.fnt");

    templini = new INIFile("templates.ini");

    printf("Loading all tiles, this can take a while...");
    fflush(stdout);

    tmps = new TemplateData*[TEMPLATES];

    for( i = 0; i < TEMPLATES; i++ ) {
        tmps[i] = new TemplateData(templini, i);
    }
    printf(" done\n");

    delete templini;
    currentTmp = 0;
    trans = new TemplateImage("trans.icn", -1);
}

TmpIniEd::~TmpIniEd()
{
    Uint32 i;
    delete fnt;
    delete gfx;
    for( i = 0; i < TEMPLATES; i++ ) {
        delete tmps[i];
    }
    delete[] tmps;
    delete trans;
}

void TmpIniEd::run()
{
    SDL_Event event;
    Uint8 done = 0;
    bool image_updated = true;
    Uint32 curpos = 0;

    while(!done) {
        if(SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_KEYDOWN:
                if( event.key.state != SDL_PRESSED )
                    break;
                switch( event.key.keysym.sym ) {
                case SDLK_0:
                    tmps[currentTmp]->setType(curpos, 0);
                    image_updated = true;
                    break;
                case SDLK_1:
                    tmps[currentTmp]->setType(curpos, 1);
                    image_updated = true;
                    break;
                case SDLK_2:
                    tmps[currentTmp]->setType(curpos, 2);
                    image_updated = true;
                    break;
                case SDLK_3:
                    tmps[currentTmp]->setType(curpos, 3);
                    image_updated = true;
                    break;
                case SDLK_4:
                    tmps[currentTmp]->setType(curpos, 4);
                    image_updated = true;
                    break;
                case SDLK_5:
                    tmps[currentTmp]->setType(curpos, 5);
                    image_updated = true;
                    break;
                case SDLK_6:
                    tmps[currentTmp]->setType(curpos, 6);
                    image_updated = true;
                    break;
                case SDLK_7:
                    tmps[currentTmp]->setType(curpos, 7);
                    image_updated = true;
                    break;
                case SDLK_RIGHT:
                    curpos++;
                    if( curpos >= tmps[currentTmp]->getNumTiles() ) {
                        curpos = 0;
                    }
                    image_updated = true;
                    break;
                case SDLK_LEFT:
                    curpos--;
                    if( curpos &0x80000000 ) {
                        curpos = tmps[currentTmp]->getNumTiles()-1;
                    }
                    image_updated = true;
                    break;
                case SDLK_UP:
                    curpos-=tmps[currentTmp]->getWidth();
                    if( curpos &0x80000000 ) {
                        curpos = 0;
                    }
                    image_updated = true;
                    break;
                case SDLK_DOWN:
                    curpos+=tmps[currentTmp]->getWidth();
                    if( curpos >= tmps[currentTmp]->getNumTiles() ) {
                        curpos = tmps[currentTmp]->getNumTiles()-1;
                    }
                    image_updated = true;
                    break;
                case SDLK_PLUS:
                    tmps[currentTmp]->increaseWidth();
                    image_updated = true;
                    break;
                case SDLK_MINUS:
                    tmps[currentTmp]->decreaseWidth();
                    image_updated = true;
                    break;
                case SDLK_n:
                    currentTmp++;
                    if( currentTmp >= TEMPLATES ) {
                        currentTmp = 0;
                    }
                    curpos = 0;
                    image_updated = true;
                    break;
                case SDLK_p:
                    currentTmp--;
                    if( currentTmp &0x80000000 ) {
                        currentTmp = TEMPLATES-1;
                    }
                    curpos = 0;
                    image_updated = true;
                    break;
                case SDLK_s:
                    saveIni();
                    break;
                default:
                    break;
                }
                break;
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
            if( image_updated ) {
                gfx->draw(tmps[currentTmp], trans, fnt, curpos);
                image_updated = false;
            }
        }
    }
}

int main(int argc, char **argv)
{
    TmpIniEd *tie;

    if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        fprintf( stderr, "Couldn't initialize SDL: %s\n", SDL_GetError() );
        exit(1);
    }
    const string& binpath = determineBinaryLocation(argv[0]);
    string lf(binpath); lf += "/tmpinied.log";
    VFS_PreInit(binpath.c_str());
    logger = new Logger(lf.c_str(),0);
    VFS_Init(binpath.c_str());
    tie = new TmpIniEd();
    tie->run();
    delete tie;
    VFS_Destroy();
    SDL_Quit();
    exit(0);
}

void loadPal(char *palname)
{
    VFile *mixfile;
    //Uint32 offset, size;
    SDL_Color palette[256];
    int i;

    mixfile = VFS_Open(palname);
    /* Load the palette */
    for (i = 0; i < 256; i++) {
        mixfile->readByte(&palette[i].r, 1);
        mixfile->readByte(&palette[i].g, 1);
        mixfile->readByte(&palette[i].b, 1);

        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
    }
    SHPBase::setPalette(palette);

}

void TmpIniEd::saveIni()
{
    Uint32 i;
    FILE *inifile;
    inifile = fopen("templates.ini", "w");
    fputs(";tiletype%d can be:\n; 0, land (default)\n; 1, water\n; 2, road\n; 3, rock\n; 4, tree\n; 5, water(blocked)\n; 7, other non passable terrain thing\n\n", inifile);

    for( i = 0; i < TEMPLATES; i++ ) {
        tmps[i]->writeData(inifile);
    }
    fclose(inifile);
}

void TemplateData::writeData(FILE *inifile)
{
    Uint32 i;
    fprintf(inifile, "[%s]\nName=%s\n", tmpnumname, name);
    fprintf(inifile, "width=%d\nheight=%d\n", width, height);
    for( i = 0; i < numtiles; i++ ) {
        if( types[i] != 0 ) {
            fprintf(inifile, "tiletype%d=%d\n", i, types[i]);
        }
    }
    fputs("\n", inifile);
}
