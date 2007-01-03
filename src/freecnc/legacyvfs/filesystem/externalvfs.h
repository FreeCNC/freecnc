#ifndef _LEGACYVFS_EXTERNALVFS_EXTERNALVFS_H
#define _LEGACYVFS_EXTERNALVFS_EXTERNALVFS_H

#include "../../freecnc.h"
#include "../archive.h"

class ExternalFiles : public Archive
{
public:
    ExternalFiles(const char *defpath);
    const char *getArchiveType() const {
        return "external file";
    }
    bool loadArchive(const char *fname);
    // Can't use default argument as we need exact type signature for
    // inheritence.
    unsigned int getFile(const char* fname) {return getFile(fname, "rb");}
    unsigned int getFile(const char *fname, const char* mode);
    void releaseFile(unsigned int file);

    unsigned int readByte(unsigned int file, unsigned char *databuf, unsigned int numBytes);
    unsigned int readWord(unsigned int file, unsigned short *databuf, unsigned int numWords);
    unsigned int readThree(unsigned int file, unsigned int *databuf, unsigned int numThrees);
    unsigned int readDWord(unsigned int file, unsigned int *databuf, unsigned int numDWords);
    char *readLine(unsigned int file, char *databuf, unsigned int buflen);

    unsigned int writeByte(unsigned int file, const unsigned char *databuf, unsigned int numBytes);
    unsigned int writeWord(unsigned int file, const unsigned short *databuf, unsigned int numWords);
    unsigned int writeThree(unsigned int file, const unsigned int *databuf, unsigned int numThrees);
    unsigned int writeDWord(unsigned int file, const unsigned int *databuf, unsigned int numDWords);
    void writeLine(unsigned int file, const char *databuf);
    int vfs_printf(unsigned int file, const char* fmt, va_list ap);
    void flush(unsigned int file);

    void seekSet(unsigned int file, unsigned int pos);
    void seekCur(unsigned int file, int pos);

    unsigned int getPos(unsigned int file) const;
    unsigned int getSize(unsigned int file) const;
    const char* getPath(unsigned int file) const;
private:
    std::string defpath;
    std::vector<std::string> path;
};

#endif
