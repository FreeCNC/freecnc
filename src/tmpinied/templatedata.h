#ifndef TEMPLATEDATA_H
#define TEMPLATEDATA_H
#include "shpimage.h"
#include "inifile.h"

class TemplateData
{
public:
    TemplateData(INIFile *tmpini, unsigned int tmpnum);
    ~TemplateData();
    unsigned int getWidth()
    {
        return width;
    }
    unsigned int getHeight()
    {
        return height;
    }
    void increaseWidth();
    void decreaseWidth();
    unsigned int getNumTiles()
    {
        return numtiles;
    }
    unsigned char getType(unsigned int tile)
    {
        return types[tile];
    }
    void setType(unsigned int tile, unsigned char type)
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
    unsigned int width;
    unsigned int height;
    unsigned int numtiles;
    unsigned char *types;
    char *name;
    char tmpnumname[16];
};


#endif
