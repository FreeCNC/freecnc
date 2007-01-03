#ifndef _LEGACYVFS_ARCHIVE_H
#define _LEGACYVFS_ARCHIVE_H

#include <cstdarg>

#include "../freecnc.h"

class Archive
{
public:
    virtual ~Archive() {}
    virtual const char *getArchiveType() const = 0;

    // Archive 
    virtual bool loadArchive(const char *fname) = 0;
    virtual void unloadArchives() {};

    // File management
    virtual unsigned int getFile(const char *fname) = 0;
    virtual void releaseFile(unsigned int file) = 0;

    // Reading
    virtual unsigned int readByte(unsigned int file, unsigned char *databuf, unsigned int numBytes) = 0;
    virtual unsigned int readWord(unsigned int file, unsigned short *databuf, unsigned int numWords) = 0;
    virtual unsigned int readThree(unsigned int file, unsigned int *databuf, unsigned int numThrees) = 0;
    virtual unsigned int readDWord(unsigned int file, unsigned int *databuf, unsigned int numDWords) = 0;
    virtual char *readLine(unsigned int file, char *databuf, unsigned int buflen) = 0;
    
    // Writing - these are not required, and only supported by the filesystem archives.
    virtual unsigned int writeByte(unsigned int file, const unsigned char* databuf, unsigned int numBytes) {return 0;}
    virtual unsigned int writeWord(unsigned int file, const unsigned short* databuf, unsigned int numWords) {return 0;}
    virtual unsigned int writeThree(unsigned int file, const unsigned int* databuf, unsigned int numThrees) {return 0;}
    virtual unsigned int writeDWord(unsigned int file, const unsigned int* databuf, unsigned int numDWords) {return 0;}
    virtual void writeLine(unsigned int file, const char* databuf) {}
    virtual int vfs_printf(unsigned int file, const char* fmt, va_list ap) {return 0;}
    virtual void flush(unsigned int file) {}

    // File position
    virtual void seekSet(unsigned int file, unsigned int pos) = 0;
    virtual void seekCur(unsigned int file, int pos) = 0;
    virtual unsigned int getPos(unsigned int file) const = 0;

    // File info
    virtual unsigned int getSize(unsigned int file) const = 0;
    virtual const char* getPath(unsigned int filenum) const = 0;
};

#endif
