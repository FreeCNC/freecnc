#ifndef _VFS_ARCHIVE_H
#define _VFS_ARCHIVE_H

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
    virtual Uint32 getFile(const char *fname) = 0;
    virtual void releaseFile(Uint32 file) = 0;

    // Reading
    virtual Uint32 readByte(Uint32 file, Uint8 *databuf, Uint32 numBytes) = 0;
    virtual Uint32 readWord(Uint32 file, Uint16 *databuf, Uint32 numWords) = 0;
    virtual Uint32 readThree(Uint32 file, Uint32 *databuf, Uint32 numThrees) = 0;
    virtual Uint32 readDWord(Uint32 file, Uint32 *databuf, Uint32 numDWords) = 0;
    virtual char *readLine(Uint32 file, char *databuf, Uint32 buflen) = 0;
    
    // Writing - these are not required, and only supported by the filesystem archives.
    virtual Uint32 writeByte(Uint32 file, const Uint8* databuf, Uint32 numBytes) {return 0;}
    virtual Uint32 writeWord(Uint32 file, const Uint16* databuf, Uint32 numWords) {return 0;}
    virtual Uint32 writeThree(Uint32 file, const Uint32* databuf, Uint32 numThrees) {return 0;}
    virtual Uint32 writeDWord(Uint32 file, const Uint32* databuf, Uint32 numDWords) {return 0;}
    virtual void writeLine(Uint32 file, const char* databuf) {}
    virtual int vfs_printf(Uint32 file, const char* fmt, va_list ap) {return 0;}
    virtual void flush(Uint32 file) {}

    // File position
    virtual void seekSet(Uint32 file, Uint32 pos) = 0;
    virtual void seekCur(Uint32 file, Sint32 pos) = 0;
    virtual Uint32 getPos(Uint32 file) const = 0;

    // File info
    virtual Uint32 getSize(Uint32 file) const = 0;
    virtual const char* getPath(Uint32 filenum) const = 0;
};

#endif
