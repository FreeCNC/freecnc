#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include "../freecnc.h"
#include "../vfs/vfs_public.h"
#include "inifile.h"

using std::runtime_error;
using boost::lexical_cast;

/** Use inside a loop to read all keys of a section.  Do not depend on the
 * order in which keys are read.
 *
 * @param section The name of the section from which to read.
 * @param keynum Will skip (keynum-1) entries in section.
 * @returns an iterator to the keynum'th key in section.
 */
INIKey INIFile::readKeyValue(const char* section, unsigned int keynum)
{
    map<string, INISection>::iterator sec;
    INIKey key;

    string s = section;
    transform(s.begin(),s.end(), s.begin(), toupper);

    sec = inidata.find(s);
    if (sec == inidata.end()) {
        throw 0;
    }

    if (keynum >= sec->second.size()) {
        throw 0;
    }

    key = sec->second.begin();
    for (unsigned int i = 0; i < keynum; ++i) {
        key++;
    }
    if (key == sec->second.end())
        throw 0;

    return key;
}

/** Use inside a loop to read all keys in a section when extraction in numeric
 * order is required, and keys are entirely numeric.
 * @param section The name of the section from which to read.
 * @param index The index of the key to extract.
 * @returns an iterator to the key with the numeric value of index.
 */
INIKey INIFile::readIndexedKeyValue(const char* section, unsigned int index, const char* prefix)
{
    map<string, INISection>::iterator sec;
    INIKey key;

    string s = section;
    transform(s.begin(),s.end(), s.begin(), toupper);

    sec = inidata.find(s);
    if (sec == inidata.end()) {
        throw 0;
    }

    if (index > sec->second.size()) {
        throw 0;
    }

    string keyval;
    if (prefix)
        keyval = prefix;
    keyval += lexical_cast<string>(index);
    key = sec->second.find(keyval);
    if (key == sec->second.end()) {
        throw 0;
    }
    return key;
}


string INIFile::readSection(unsigned int secnum)
{
    map<string, INISection>::iterator sec;
    unsigned int i;

    if (secnum >= inidata.size()) {
        throw 0;
    }
    sec = inidata.begin();
    for (i = 0; i < secnum; i++) {
        sec++;
        if (sec == inidata.end()) {
            throw 0;
        }
    }
    return sec->first;
}

/** Function to extract a string from a ini file. The string
 * is malloced in this function so it should be freed.
 * @param the section in the file to extract string from.
 * @param the name of the string to extract.
 * @return the extracted string.
 */
char* INIFile::readString(const char* section, const char* value)
{
    char* retval;
    map<string, INISection>::iterator sec;
    INIKey key;

    string s = section;
    transform (s.begin(),s.end(), s.begin(), toupper);
    string v = value;
    transform (v.begin(),v.end(), v.begin(), toupper);

    sec = inidata.find(s);
    if (sec == inidata.end()) {
        return NULL;
    }

    key = sec->second.find(v);
    if (key == sec->second.end()) {
        return NULL;
    }
    retval = new char[key->second.length()+1];
    return strcpy(retval, key->second.c_str());
}

/// wrapper around readString to return a provided default instead of NULL
char* INIFile::readString(const char* section, const char* value, const char* deflt)
{
    char* tmp;

    tmp = readString(section,value);
    if (tmp == NULL) {
        /* a new string is allocated because this guarentees
         * that the return value can be delete[]ed safely 
         */
        tmp = cppstrdup(deflt);
    }
    return tmp;
}

/** Function to extract a integer value from a ini file. The value
 * can be given in hex if it starts with 0x.
 * @param the section in the file to extract values from.
 * @param the name of the value to extract.
 * @return the value.
 */
int INIFile::readInt(const char* section, const char* value)
{
    int retval;
    map<string, INISection>::iterator sec;
    INIKey key;

    string s = section;
    transform (s.begin(),s.end(), s.begin(), toupper);
    string v = value;
    transform (v.begin(),v.end(), v.begin(), toupper);

    sec = inidata.find(s);
    if (sec == inidata.end()) {
        return INIERROR;
    }

    key = sec->second.find(v);
    if (key == sec->second.end()) {
        return INIERROR;
    }
    if (sscanf(key->second.c_str(), "%d", &retval) != 1) {
        return INIERROR;
    }
    return retval;
}

/// wrapper around readInt to return a provided default instead of INIERROR
int INIFile::readInt(const char* section, const char* value, unsigned int deflt)
{
    unsigned int tmp;

    tmp = readInt(section,value);
    if (tmp == INIERROR)
        return deflt;
    else
        return tmp;
}

/** Constructor, opens the file
 * @param the name of the inifile to open.
 */
INIFile::INIFile(const char* filename)
{
    char line[1024];
    char key[1024];
    char value[1024];
    unsigned int i;
    char* str;

    VFile* inifile;
    string cursectionName;
    INISection cursection;

    inifile = VFS_Open(filename);

    if (inifile == NULL) {
        string s = "Unable to open ";
        s += filename;
        throw runtime_error(s);
    }

    cursectionName = "";

    // parse the inifile and write data to inidata
    while (inifile->getLine(line, 1024) != NULL) {
        str = line;
        while ((*str) == ' ' || (*str) == '\t') {
            str++;
        }
        if ((*str) == ';') {
            continue;
        }
        if (sscanf(str, "[%[^]]]", key) == 1) {
            if (cursectionName != "") {
                inidata[cursectionName] = cursection;
                cursection.clear();
            }
            for (i = 0; key[i] != '\0'; i++) {
                key[i] = toupper(key[i]);
            }
            cursectionName = key;
        } else if (cursectionName != "" &&
                   sscanf(str, "%[^=]=%[^\r\n;]", key, value) == 2) {
            for (i = 0; key[i] != '\0'; i++) {
                key[i] = toupper(key[i]);
            }
            if (strlen(key) > 0) {
                str = key+strlen(key)-1;
                while ((*str) == ' ' || (*str) == '\t') {
                    (*str) = '\0';
                    if (str == key) {
                        break;
                    }
                    str--;
                }
            }
            if (strlen(value) > 0) {
                str = value+strlen(value)-1;
                while ((*str) == ' ' || (*str) == '\t') {
                    (*str) = '\0';
                    if (str == value) {
                        break;
                    }
                    str--;
                }
            }
            str = value;
            while ((*str) == ' ' || (*str) == '\t') {
                str++;
            }
            cursection[(string)key] = (string)str;
        }
    }
    if (cursectionName != "") {
        inidata[cursectionName] = cursection;
        cursection.clear();
    }

    VFS_Close(inifile);
}

/** Destructor, closes the file */
INIFile::~INIFile()
{
    // delete all entries in inidata
}

