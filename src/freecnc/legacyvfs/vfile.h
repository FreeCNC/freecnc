#ifndef _LEGACYVFS_VFILE_H
#define _LEGACYVFS_VFILE_H

#include "../freecnc.h"
#include "archive.h"

class VFile
{
public:
    VFile(unsigned int filenum, Archive *arch)
        : filenum(filenum), archive(arch)
    {}

    ~VFile()
    {
        archive->releaseFile(filenum);
    }

    unsigned int readByte(unsigned char *databuf, unsigned int numBytes)
    {
        return archive->readByte(filenum, databuf, numBytes);
    }

    unsigned int readWord(unsigned short *databuf, unsigned int numWords)
    {
        return archive->readWord(filenum, databuf, numWords);
    }

    unsigned int readThree(unsigned int *databuf, unsigned int numThrees)
    {
        return archive->readThree(filenum, databuf, numThrees);
    }

    unsigned int readDWord(unsigned int *databuf, unsigned int numDWords)
    {
        return archive->readDWord(filenum, databuf, numDWords);
    }

    char *getLine(char *string, unsigned int buflen)
    {
        return archive->readLine(filenum, string, buflen);
    }

    unsigned int writeByte(unsigned char* databuf, unsigned int numBytes)
    {
        return archive->writeByte(filenum, databuf, numBytes);
    }

    unsigned int writeWord(unsigned short *databuf, unsigned int numWords)
    {
        return archive->writeWord(filenum, databuf, numWords);
    }

    unsigned int writeThree(unsigned int *databuf, unsigned int numThrees)
    {
        return archive->writeThree(filenum, databuf, numThrees);
    }

    unsigned int writeDWord(unsigned int *databuf, unsigned int numDWords)
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

    void seekSet(unsigned int pos)
    {
        archive->seekSet(filenum, pos);
    }

    void seekCur(int offset)
    {
        archive->seekCur(filenum, offset);
    }

    unsigned int fileSize()
    {
        return archive->getSize(filenum);
    }

    unsigned int tell()
    {
        return archive->getPos(filenum);
    }

    const char *getPath()
    {
        return archive->getPath(filenum);
    }

private:
    unsigned int filenum;
    Archive *archive;
};

#endif
