#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include "../freecnc.h"
#include "../legacyvfs/vfs_public.h"
#include "inifile.h"

using std::runtime_error;
using boost::lexical_cast;
using boost::to_upper;
using boost::trim;

namespace
{
    typedef boost::tokenizer<boost::char_separator<char> > LineTokenizer;
    boost::char_separator<char> keyvalue_sep("=");
    boost::char_separator<char> section_sep("[]");
}

INIFile::INIFile(const string& filename)
{
    string cursection_name;
    INISection cursection;

    shared_ptr<File> inifile = game.vfs.open(filename);

    if (inifile == NULL) {
        throw INIFileNotFound("INIFile: Unable to open" + filename);
    }

    int linenum = 0;
    // parse the inifile and write data to inidata
    for (string line; !inifile->eof(); line = inifile->readline()) {
        const char* str = line.c_str();
        while ((*str) == ' ' || (*str) == '\t') {
            str++;
        }
        if ((*str) == ';') {
            continue;
        }

        if (*str == '[') {
            // This isn't perfect, but it's better than what was before...
            LineTokenizer section_name(line, section_sep);
            if (std::distance(section_name.begin(), section_name.end()) == 1) {
                if (cursection_name != "") {
                    inidata[cursection_name] = cursection;
                    cursection.clear();
                }
                cursection_name = *section_name.begin();
                to_upper(cursection_name);
                trim(cursection_name);
            }
        } else if (cursection_name != "") {
            LineTokenizer data(line, keyvalue_sep);

            if (std::distance(data.begin(), data.end()) != 2) {
                game.log << "INIFile: Syntax error at line " << linenum << endl;
                continue;
            }
            LineTokenizer::iterator it(data.begin());

            string key = *it++;
            to_upper(key);
            trim(key);

            string value = *it++;
            trim(value);

            if ((key.length() == 0) || (value.length() == 0)) {
                game.log << "INIFile: Syntax error at line " << linenum << endl;
                continue;
            }
            cursection[key] = value;
        }
        ++linenum;
    }
    if (cursection_name != "") {
        inidata[cursection_name] = cursection;
    }
}

// Deprecated
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

INISection* INIFile::section(string section)
{
    to_upper(section);
    map<string, INISection>::iterator sec = inidata.find(section);
    if (sec == inidata.end()) {
        return NULL;
    }
    return &sec->second;
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
