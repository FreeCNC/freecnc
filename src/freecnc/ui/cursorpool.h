#ifndef _UI_CURSORPOOL_H
#define _UI_CURSORPOOL_H

#include "../freecnc.h"

class INIFile;

struct CursorInfo
{
    unsigned short anstart,anend;
};

class CursorPool
{
private:
    vector<CursorInfo*> cursorpool;
    map<string, unsigned short> name2index;
    shared_ptr<INIFile> cursorini;
public:
    CursorPool(const char* ininame);
    ~CursorPool();
    CursorInfo* getCursorByName(const char* name);
};

#endif
