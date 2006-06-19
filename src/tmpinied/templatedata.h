#ifndef TEMPLATEDATA_H
#define TEMPLATEDATA_H
#include "shpimage.h"
#include "inifile.h"

class TemplateData
{
public:
    TemplateData(INIFile *tmpini, Uint32 tmpnum);
    ~TemplateData();
    Uint32 getWidth()
    {
        return width;
    }
    Uint32 getHeight()
    {
        return height;
    }
    void increaseWidth();
    void decreaseWidth();
    Uint32 getNumTiles()
    {
        return numtiles;
    }
    Uint8 getType(Uint32 tile)
    {
        return types[tile];
    }
    void setType(Uint32 tile, Uint8 type)
    {
        types[tile]=type;
    }
    char *getName()
    {
        return name;
    }
    TemplateImage *getImage();
    char *getNumName()
    {
        return tmpnumname;
    }
    void writeData(FILE *inifile);
private:
    Uint32 width;
    Uint32 height;
    Uint32 numtiles;
    Uint8 *types;
    char *name;
    char tmpnumname[16];
};


#endif
