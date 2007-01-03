#ifndef _VFS_MIX_MIX_H
#define _VFS_MIX_MIX_H

#include "../../freecnc.h"
#include "../archive.h"

class VFile;

namespace MIXPriv
{
    template<class T>
    inline T ROL(T n)
    {
        return ((n << 1) | ((n >> 31) & 1));
    }

    // Magic constants
    const unsigned int TS_ID = 0x763c81dd;
    const unsigned long int mix_checksum = 0x00010000;
    const unsigned long int mix_encrypted = 0x00020000;

    /* data type for which game the specific
     * mix file is from. Different from the
     * game definitions in freecnc.h
     */
    enum game_t {game_td, game_ra, game_ts};
    
    // d = first 4 bytes of mix file
    inline game_t which_game(unsigned int d)
    {
        return (d == 0 || d == mix_checksum || d == mix_encrypted || d == (mix_encrypted | mix_checksum)) ? game_ra : game_td;
    }

    #if __GNUC__
    #define FCNC_PACKED __attribute__ ((packed))
    #endif

    #ifdef _MSC_VER
    #pragma pack(push, 1)
    #define FCNC_PACKED
    #endif
    struct MixHeader {
        unsigned short c_files;
        unsigned int size FCNC_PACKED;
    };
    #ifdef _MSC_VER
    #undef FCNC_PACKED
    #pragma pack(pop)
    #endif

    struct MixRecord {
        unsigned int id;
        unsigned int offset;
        unsigned int size;
    };

    /* only 256 mixfiles can be loaded */
    struct MIXEntry {
        unsigned char filenum;
        unsigned int offset;
        unsigned int size;
    };

    struct OpenFile {
        unsigned int id;
        unsigned int pos;
    };

    enum tscheck_ {check_ts, nocheck_ts};

    typedef std::map<unsigned int, MIXEntry> mixheaders_t;
    typedef std::map<unsigned int, OpenFile> openfiles_t;
}

class MIXFiles : public Archive {
public:
    MIXFiles();
    ~MIXFiles();
    const char *getArchiveType() const {
        return "mix archive";
    }
    bool loadArchive(const char *fname);
    void unloadArchives();
    unsigned int getFile(const char *fname);
    void releaseFile(unsigned int file);

    unsigned int readByte(unsigned int file, unsigned char *databuf, unsigned int numBytes);
    unsigned int readWord(unsigned int file, unsigned short *databuf, unsigned int numWords);
    unsigned int readThree(unsigned int file, unsigned int *databuf, unsigned int numThrees);
    unsigned int readDWord(unsigned int file, unsigned int *databuf, unsigned int numDWords);
    char *readLine(unsigned int file, char *databuf, unsigned int buflen);

    void seekSet(unsigned int file, unsigned int pos);
    void seekCur(unsigned int file, int pos);

    //unsigned int getStartpos(unsigned int file){return 0;}
    unsigned int getPos(unsigned int file) const;
    unsigned int getSize(unsigned int file) const;

    const char* getPath(unsigned int file) const;
private:
    unsigned int calcID(const char* fname );
    void readMIXHeader(VFile* mix);
    MIXPriv::MixRecord* decodeHeader(VFile* mix, MIXPriv::MixHeader* header,
            MIXPriv::tscheck_ tscheck);

    std::vector<VFile*> mixfiles;
    MIXPriv::mixheaders_t mixheaders;

    MIXPriv::openfiles_t openfiles;
};

#endif
