#ifndef _LIB_INIFILE_H
#define _LIB_INIFILE_H

#include "../basictypes.h"

#define MAXLINELENGTH 1024
#define MAXSTRINGLENGTH 128
#define INIERROR 0x7fffffff

typedef std::map<std::string,std::string> INISection;
typedef INISection::const_iterator INIKey;
typedef INISection::value_type INISectionItem;

class INIFile
{
public:
    explicit INIFile(const char* filename);

    /// @TODO Would be nice if there was a version that returned a non-copy.
    char* readString(const char* section, const char* value);
    char* readString(const char* section, const char* value, const char* deflt);

    int readInt(const char* section, const char* value, unsigned int deflt);
    int readInt(const char* section, const char* value);

    // Take copy to upper case
    INISection* section(string section);

    INIKey readKeyValue(const char* section, unsigned int keynum);
    INIKey readIndexedKeyValue(const char* section, unsigned int keynum, const char* prefix=0);
    std::string readSection(unsigned int secnum);
private:
    std::map<std::string, INISection> inidata;
};

#endif
