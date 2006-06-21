#ifndef _VFS_VFILE_H
#define _VFS_VFILE_H

#include "../freecnc.h"
#include "archive.h"

class VFile
{
public:
    VFile(Uint32 filenum, Archive *arch)
        : filenum(filenum), archive(arch)
    {}

    ~VFile()
    {
        archive->releaseFile(filenum);
    }

    Uint32 readByte(Uint8 *databuf, Uint32 numBytes)
    {
        return archive->readByte(filenum, databuf, numBytes);
    }

    Uint32 readWord(Uint16 *databuf, Uint32 numWords)
    {
        return archive->readWord(filenum, databuf, numWords);
    }

    Uint32 readThree(Uint32 *databuf, Uint32 numThrees)
    {
        return archive->readThree(filenum, databuf, numThrees);
    }

    Uint32 readDWord(Uint32 *databuf, Uint32 numDWords)
    {
        return archive->readDWord(filenum, databuf, numDWords);
    }

    char *getLine(char *string, Uint32 buflen)
    {
        return archive->readLine(filenum, string, buflen);
    }

    Uint32 writeByte(Uint8* databuf, Uint32 numBytes)
    {
        return archive->writeByte(filenum, databuf, numBytes);
    }

    Uint32 writeWord(Uint16 *databuf, Uint32 numWords)
    {
        return archive->writeWord(filenum, databuf, numWords);
    }

    Uint32 writeThree(Uint32 *databuf, Uint32 numThrees)
    {
        return archive->writeThree(filenum, databuf, numThrees);
    }

    Uint32 writeDWord(Uint32 *databuf, Uint32 numDWords)
    {
        return archive->writeDWord(filenum, databuf, numDWords);
    }

    void writeLine(const char* string)
    {
        archive->writeLine(filenum, string);
    }

    int vfs_printf(const char* fmt, ...)
    {
        int ret;
        va_list ap;
        va_start(ap, fmt);
        ret = archive->vfs_printf(filenum, fmt, ap);
        va_end(ap);
        return ret;
    }

    int vfs_vprintf(const char* fmt, va_list ap)
    {
        return archive->vfs_printf(filenum, fmt, ap);
    }

    void flush()
    {
        archive->flush(filenum);
    }

    void seekSet(Uint32 pos)
    {
        archive->seekSet(filenum, pos);
    }

    void seekCur(Sint32 offset)
    {
        archive->seekCur(filenum, offset);
    }

    Uint32 fileSize()
    {
        return archive->getSize(filenum);
    }

    Uint32 tell()
    {
        return archive->getPos(filenum);
    }

    const char *getPath()
    {
        return archive->getPath(filenum);
    }

private:
    Uint32 filenum;
    Archive *archive;
};

#endif
