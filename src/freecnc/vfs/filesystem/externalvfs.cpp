#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <cerrno>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include "externalvfs.h"

namespace ExtPriv {
struct OpenFile {
    FILE *file;
    size_t size;
    std::string path;
};
typedef std::map<size_t, OpenFile> openfiles_t;
openfiles_t openfiles;
FILE* fcaseopen(std::string* path, const char* mode, unsigned int caseoffset = 0) throw();
bool isdir(const string& path);
}

using namespace ExtPriv; // XXX:  This compiles, namespace ExtPriv {...} doesn't.

ExternalFiles::ExternalFiles(const char *defpath) : defpath(defpath) {
}

bool ExternalFiles::loadArchive(const char *fname)
{
    string pth(fname);
    if ("." == pth || "./" == pth) {
        path.push_back("./");
        return true;
    }
    if (isRelativePath(fname)) {
        pth = defpath + "/" + fname;
    } else {
        pth = fname;
    }
    if ('/' != pth[pth.length() - 1]) {
        pth += "/";
    }
    if (!isdir(pth)) {
        return false;
    }
    path.push_back(pth);
    return true;
}

unsigned int ExternalFiles::getFile(const char *fname, const char* mode)
{
    ExtPriv::OpenFile newFile;
    FILE *f;
    unsigned int i;
    string filename;
    size_t size, fnum;

    if (mode[0] != 'r') {
        filename = fname;
        f = fopen(filename.c_str(), mode);
        if (f != NULL) {
            newFile.file = f;
            // We'll just ignore file sizes for files being written for now.
            newFile.size = 0;
            newFile.path = filename;
            fnum = (size_t)f;
            openfiles[fnum] = newFile;
            return fnum;
        } // Error condition hanled at end of function
    }
    for (i = 0; i < path.size(); ++i) {
        filename = path[i] + fname;
        f = fcaseopen(&filename, mode, path[i].length());
        if (f != NULL) {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fseek(f, 0, SEEK_SET);
            newFile.file = f;
            newFile.size = size;
            newFile.path = filename;

            fnum = (size_t)f;
            openfiles[fnum] = newFile;
            return fnum;
        }
    }

    return (unsigned int)-1;
}

void ExternalFiles::releaseFile(unsigned int file)
{
    fclose(openfiles[file].file);
    openfiles.erase(file);
}

unsigned int ExternalFiles::readByte(unsigned int file, unsigned char *databuf, unsigned int numBytes)
{
    return fread(databuf, 1, numBytes, openfiles[file].file);
}

unsigned int ExternalFiles::readWord(unsigned int file, unsigned short *databuf, unsigned int numWords)
{
    unsigned int numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    unsigned int i;
#endif

    numRead = fread(databuf, 2, numWords, openfiles[file].file);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap16(databuf[i]);
    }
#endif

    return numRead;
}

unsigned int ExternalFiles::readThree(unsigned int file, unsigned int *databuf, unsigned int numThrees)
{
    unsigned int numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    unsigned int i;
#endif

    numRead = fread(databuf, 3, numThrees, openfiles[file].file);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap32(databuf[i]);
        databuf[i]<<=8;
    }
#endif

    return numRead;
}

unsigned int ExternalFiles::readDWord(unsigned int file, unsigned int *databuf, unsigned int numDWords)
{
    unsigned int numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    unsigned int i;
#endif

    numRead = fread(databuf, 4, numDWords, openfiles[file].file);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap32(databuf[i]);
    }
#endif

    return numRead;
}

char *ExternalFiles::readLine(unsigned int file, char *databuf, unsigned int buflen)
{
    return fgets(databuf, buflen, openfiles[file].file);
}

unsigned int ExternalFiles::writeByte(unsigned int file, const unsigned char* databuf, unsigned int numBytes)
{
    return fwrite(databuf, 1, numBytes, openfiles[file].file);
}

unsigned int ExternalFiles::writeWord(unsigned int file, const unsigned short *databuf, unsigned int numWords)
{
    unsigned int numWrote;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    unsigned short* tmp = new unsigned short[numWords];
    unsigned int i;

    for( i = 0; i < numWords; i++ ) {
        tmp[i] = SDL_Swap16(databuf[i]);
    }

    numWrote = fwrite(tmp, 2, numWords, openfiles[file].file);
    delete[] tmp;
#else

    numWrote = fwrite(databuf, 2, numWords, openfiles[file].file);
#endif

    return numWrote;
}

unsigned int ExternalFiles::writeThree(unsigned int file, const unsigned int *databuf, unsigned int numThrees)
{
    unsigned int numWrote;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    unsigned int* tmp = new unsigned int[numThrees];
    unsigned int i;

    for( i = 0; i < numThrees; i++ ) {
        tmp[i] = SDL_Swap32(databuf[i]);
        tmp[i]<<=8;
    }
    numWrote = fwrite(tmp, 3, numThrees, openfiles[file].file);
    delete[] tmp;
#else

    numWrote = fwrite(databuf, 3, numThrees, openfiles[file].file);
#endif

    return numWrote;
}

unsigned int ExternalFiles::writeDWord(unsigned int file, const unsigned int *databuf, unsigned int numDWords)
{
    unsigned int numWrote;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    unsigned int i;
    unsigned int* tmp = new unsigned int[numDWords];

    for( i = 0; i < numDWords; i++ ) {
        tmp[i] = SDL_Swap32(databuf[i]);
    }
    numWrote = fwrite(tmp, 4, numDWords, openfiles[file].file);
    delete[] tmp;
#else

    numWrote = fwrite(databuf, 4, numDWords, openfiles[file].file);
#endif

    return numWrote;
}

void ExternalFiles::writeLine(unsigned int file, const char *databuf)
{
    fputs(databuf, openfiles[file].file);
}

int ExternalFiles::vfs_printf(unsigned int file, const char* fmt, va_list ap)
{
    int ret;
    ret = vfprintf(openfiles[file].file, fmt, ap);
    return ret;
}

void ExternalFiles::flush(unsigned int file) {
    fflush(openfiles[file].file);
}

void ExternalFiles::seekSet(unsigned int file, unsigned int pos)
{
    fseek(openfiles[file].file, pos, SEEK_SET);
}

void ExternalFiles::seekCur(unsigned int file, int pos)
{
    fseek(openfiles[file].file, pos, SEEK_CUR);
}

unsigned int ExternalFiles::getPos(unsigned int file) const {
    /// @TODO Abstract this const implementation of operator[].
    openfiles_t::const_iterator i = openfiles.find(file);
    if (openfiles.end() != i) {
        return ftell(i->second.file);
    }
    /// @TODO Throw an exception in a later iteration of code cleanup.
    return 0;
}

unsigned int ExternalFiles::getSize(unsigned int file) const {
    /// @TODO Abstract this const implementation of operator[].
    openfiles_t::const_iterator i = openfiles.find(file);
    if (openfiles.end() != i) {
        return i->second.size;
    }
    /// @TODO Throw an exception in a later iteration of code cleanup.
    return 0;
}

const char* ExternalFiles::getPath(unsigned int file) const {
    /// @TODO Abstract this const implementation of operator[].
    openfiles_t::const_iterator i = openfiles.find(file);
    if (openfiles.end() != i) {
        return i->second.path.c_str();
    }
    /// @TODO Throw an exception in a later iteration of code cleanup.
    return 0;
}

namespace ExtPriv {

FILE* fcaseopen(string* name, const char* mode, unsigned int caseoffset) throw() {
    FILE* ret;
    ret = fopen(name->c_str(), mode);
    if (NULL != ret) {
        return ret;
    }
#ifdef _WIN32
    return NULL;
#else
    string& fname = *name;
    // Try all other case.  Assuming uniform casing.
    unsigned int i;
    // Skip over non-alpha chars.
    // @TODO These are the old style text munging routines that are a) consise
    // and b) doesn't work with UTF8 filenames.
    for (i=caseoffset;i<fname.length()&&!isalpha(fname[i]);++i);
    if (islower(fname[i])) {
        transform(fname.begin()+caseoffset, fname.end(), fname.begin()+caseoffset, toupper);
    } else {
        transform(fname.begin()+caseoffset, fname.end(), fname.begin()+caseoffset, tolower);
    }
    ret = fopen(fname.c_str(), mode);
    if (NULL != ret) {
        return ret;
    }
    /// @TODO Try other tricks like "lower.EXT" or "UPPER.ext"
    return NULL;
#endif
}

bool isdir(const string& path) {
#ifdef _WIN32
    DWORD length = GetCurrentDirectory(0, 0);
    char* orig_path = new char[length];
    GetCurrentDirectory(length, orig_path);
    if (!SetCurrentDirectory(path.c_str())) {
        delete[] orig_path;
        return false;
    }
    SetCurrentDirectory(orig_path);
    delete[] orig_path;
    return true;
#else
    int curdirfd = open("./", O_RDONLY);
    if (-1 == curdirfd) {
        return false;
    }
    if (-1 == chdir(path.c_str())) {
        return false;
    }
    fchdir(curdirfd);
    close(curdirfd);
#endif
    return true;
}

} // namespace ExtPriv
