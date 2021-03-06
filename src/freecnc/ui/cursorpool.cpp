#include "../lib/inifile.h"
#include "cursorpool.h"

CursorPool::CursorPool(const char* ininame)
{
    cursorini = GetConfig(ininame);
}

CursorPool::~CursorPool()
{
    for (unsigned int i = 0; i < cursorpool.size(); ++i) {
        delete cursorpool[i];
    }
}

CursorInfo* CursorPool::getCursorByName(const char* name)
{
    map<string, unsigned short>::iterator cursorentry;
    CursorInfo* datum;
    unsigned short index;

    string cname = (string)name;
    string::iterator p = cname.begin();
    while (p!=cname.end()) {
        *p = toupper(*p);
        ++p;
    }

    cursorentry = name2index.find(cname);
    if (cursorentry != name2index.end() ) {
        index = cursorentry->second;
        datum = cursorpool[index];
    } else {
        index = static_cast<unsigned short>(cursorpool.size());
        datum = new CursorInfo;
        //cursorini->seekSection(name);
        datum->anstart = cursorini->readInt(name,"start",0);
        datum->anend = cursorini->readInt(name,"end",0);
        cursorpool.push_back(datum);
        name2index[cname] = index;
    }

    return datum;
}
