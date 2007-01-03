#include <cctype>
#include "../vfs.h"
#include "../vfile.h"
#include "blowfish.h"
#include "ws-key.h"
#include "mixvfs.h"
#include "../../lib/fcncendian.h"

#ifdef _WIN32
#define EXPORT __declspec( dllexport )
#else
#define EXPORT
#endif

using namespace MIXPriv;

MIXFiles::MIXFiles() {
}

MIXFiles::~MIXFiles() {
    unloadArchives();
}

bool MIXFiles::loadArchive(const char *fname) {
    VFile *file;
    file = VFS_Open(fname);
    if (file == 0) {
        return false;
    }
    mixfiles.push_back(file);
    readMIXHeader(file);
    return true;
}

void MIXFiles::unloadArchives() {
    unsigned int i;
    for (i = 0; i < mixfiles.size(); ++i) {
        VFS_Close(mixfiles[i]);
    }
    mixfiles.resize(0);
    mixheaders.clear();
}

unsigned int MIXFiles::getFile(const char *fname) {
    VFile *myvfile;
    mixheaders_t::iterator epos;
    openfiles_t::iterator of;
    OpenFile newfile;
    unsigned int id;
    id = calcID(fname);
    epos = mixheaders.find(id);
    if (mixheaders.end() == epos) {
        return (unsigned int)-1;
    }
    myvfile = mixfiles[epos->second.filenum];

    newfile.id = id;
    newfile.pos = 0;

    openfiles_t::const_iterator ofe = openfiles.end();
    do { /// @TODO Rewrite this loop.
        of = openfiles.find(id++);
    } while (ofe != of);
    id--;

    openfiles[id] = newfile;

    return id;
}

void MIXFiles::releaseFile(unsigned int file) {
    openfiles.erase(file);
}

/** Function to calculate a idnumber from a filename
 * @param the filename
 * @return the id.
 */
unsigned int MIXFiles::calcID(const char *fname) {
    unsigned int calc;
    int i;
    char buffer[13];
#if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
    unsigned int tmpswap;
#endif
    for (i=0; *fname!='\0' && i<12; i++)
        buffer[i]=toupper(*(fname++));
    while(i<13)
        buffer[i++]=0;

    calc=0;
    for(i=0;buffer[i]!=0;i+=4) {
#if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
        tmpswap = SDL_Swap32(*(long *)(buffer+i));
        calc=ROL(calc)+tmpswap;
#else
        calc=ROL(calc)+(*(long *)(buffer+i));
#endif
    }
    return calc;
}

/** Decodes RA/TS Style MIX headers. Assumes you have already checked if
 *  header is encrypted and that mix is seeked to the start of the WSKey
 *
 * @param mix pointer to vfile for the mixfile
 * @param header pointer to header object that will store the mix's header
 * @param tscheck if equal to check_ts, will check if mix is from Tiberian Sun.
 * @return pointer to MixRecord
 */
MixRecord *MIXFiles::decodeHeader(VFile* mix, MixHeader* header, tscheck_ tscheck) {
    unsigned char WSKey[80];        /* 80-byte Westwood key */
    unsigned char BFKey[56];        /* 56-byte blow fish key */
    unsigned char Block[8];         /* 8-byte block to store blowfish stuff in */
    Cblowfish bf;
    unsigned char *e;
    MixRecord* mindex;
    //bool aligned = true;

    mix->readByte(WSKey, 80);
    get_blowfish_key((const unsigned char *)&WSKey, (unsigned char *)&BFKey);
    bf.set_key((const unsigned char *)&BFKey, 56);
    mix->readByte(Block, 8);
    bf.decipher(&Block, &Block, 8);

    // Extract the header from Block
    memcpy(&header->c_files, &Block[0], sizeof(unsigned short));
    memcpy(&header->size, &Block[sizeof(unsigned short)], sizeof(unsigned int));
#if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
    /// @TODO Verify this is still needed.
    header->c_files = SDL_Swap32(header->c_files);
    header->size = SDL_Swap32(header->size);
#endif
    /* Decrypt all indexes */
    const int m_size = sizeof(MixRecord) * header->c_files;
    const int m_f = m_size + 5 & ~7;
    mindex = new MixRecord[header->c_files];
    e = new unsigned char[m_f];
    //fread(e, m_f, 1, mix);
    mix->readByte(e, m_f);
    memcpy(mindex, &Block[6], 2);
    bf.decipher(e, e, m_f);
    memcpy(reinterpret_cast<unsigned char *>(mindex) + 2, e, m_size - 2);
    delete[] e;

    for (int i = 0; i < header->c_files; i++) {
#if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
        mindex[i].id = SDL_Swap32(mindex[i].id);
        mindex[i].offset = SDL_Swap32(mindex[i].offset);
        mindex[i].size = SDL_Swap32(mindex[i].size);
#endif
#if 0
        if (check_ts == tscheck) {
            if (mindex[i].offset & 0xf)
                aligned = false;
            if (mindex[i].id == TS_ID)
                game = game_ts;
        }
#endif
        /* 92 = 4 byte flag + 6 byte header + 80 byte key + 2 bytes (?) */
        mindex[i].offset += 92 + m_f; /* re-center offset to be absolute offset */
    }
    /*
     if (aligned) game = game_ts;
    */
    return mindex;
}


/** read the mixheader */
void MIXFiles::readMIXHeader(VFile *mix) {
    MIXEntry mentry;
    MixHeader header;
    MixRecord *m_index = NULL;
    game_t game;
    unsigned int i;
    unsigned int flags;

    // Read header

    mix->readWord(&header.c_files, 1);
    mix->readDWord(&header.size, 1);

#if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
// Don't know it this is needed.
//    header.flags = SDL_Swap32(header.flags);
#endif

    flags = header.c_files | header.size << 16;

    game = which_game(flags);
    if (game == game_ra) {
        //fseek(mix, -2, SEEK_CUR);
        mix->seekCur(-2);
        if (flags & mix_encrypted) {
            m_index = decodeHeader(mix, &header, check_ts);
        } else { /* mix is not encrypted */
            bool aligned = true;
            mix->seekSet(4);
            mix->readWord(&header.c_files, 1);
            mix->readDWord(&header.size, 1);

            const int m_size = sizeof(MixRecord) * header.c_files;
            m_index = new MixRecord[header.c_files];
            mix->readByte((unsigned char *)m_index, m_size);
            for (i = 0; i < header.c_files; i++) {
#if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
                m_index[i].id = SDL_Swap32(m_index[i].id);
                m_index[i].size = SDL_Swap32(m_index[i].size);
                m_index[i].offset = SDL_Swap32(m_index[i].offset);
#endif

                if (m_index[i].offset & 0xf)
                    aligned = false;
                if (m_index[i].id == TS_ID)
                    game = game_ts;
                m_index[i].offset += 4 + sizeof(MixHeader) + m_size;
            }
            if (aligned)
                game = game_ts;
        }
    } else if (game_td == game) {
#if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
        mix->seekSet(0);
        mix->readWord(&header.c_files, 1);
        mix->readDWord(&header.size, 1);
#endif

        const int m_size = sizeof(MixRecord) * header.c_files;
        m_index = new MixRecord[header.c_files];
        //fread(reinterpret_cast<unsigned char *>(m_index), m_size, 1, mix);
        mix->readByte((unsigned char *)m_index, m_size);
        for (i = 0; i < header.c_files; i++) {
#if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
            m_index[i].id = SDL_Swap32(m_index[i].id);
            m_index[i].offset = SDL_Swap32(m_index[i].offset);
            m_index[i].size = SDL_Swap32(m_index[i].size);
#endif
            /* 6 = 6 byte header - no other header/flags or keys in TD mixes */
            m_index[i].offset += 6 + m_size;
        }
    }
    for (i = 0; i < header.c_files; ++i) {
        mentry.filenum = (unsigned char)mixfiles.size()-1;
        mentry.offset = m_index[i].offset;
        mentry.size = m_index[i].size;
        mixheaders[m_index[i].id] = mentry;
    }
    delete[] m_index;
}

unsigned int MIXFiles::readByte(unsigned int file, unsigned char *databuf, unsigned int numBytes) {
    unsigned int numRead;
    unsigned int id, pos;
    MIXEntry me;

    id = openfiles[file].id;
    pos = openfiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(numBytes, (me.size-pos));
    numRead = mixfiles[me.filenum]->readByte(databuf, numRead);
    openfiles[file].pos += numRead;
    return numRead;
}

unsigned int MIXFiles::readWord(unsigned int file, unsigned short *databuf, unsigned int numWords) {
    unsigned int numRead;
    unsigned int id, pos;
    MIXEntry me;

    id = openfiles[file].id;
    pos = openfiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(numWords, ((me.size-pos)>>1));
    numRead = mixfiles[me.filenum]->readWord(databuf, numRead);
    openfiles[file].pos += numRead<<1;
    return numRead;
}

unsigned int MIXFiles::readThree(unsigned int file, unsigned int *databuf, unsigned int numThrees) {
    unsigned int numRead;
    unsigned int id, pos;
    MIXEntry me;

    id = openfiles[file].id;
    pos = openfiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(numThrees, ((me.size-pos)/3));
    numRead = mixfiles[me.filenum]->readThree(databuf, numRead);
    openfiles[file].pos += numRead*3;
    return numRead;
}

unsigned int MIXFiles::readDWord(unsigned int file, unsigned int *databuf, unsigned int numDWords) {
    unsigned int numRead;
    unsigned int id, pos;
    MIXEntry me;

    id = openfiles[file].id;
    pos = openfiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(numDWords, ((me.size-pos)>>2));
    numRead = mixfiles[me.filenum]->readDWord(databuf, numRead);
    openfiles[file].pos += numRead<<2;
    return numRead;
}

char *MIXFiles::readLine(unsigned int file, char *databuf, unsigned int buflen) {
    unsigned int numRead;
    unsigned int id, pos;
    MIXEntry me;
    char *retval;

    id = openfiles[file].id;
    pos = openfiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(buflen-1, me.size-pos);
    if( numRead == 0 ) {
        return NULL;
    }
    retval = mixfiles[me.filenum]->getLine(databuf, numRead+1);
    openfiles[file].pos += (unsigned int)strlen(databuf);
    return retval;
}

void MIXFiles::seekSet(unsigned int file, unsigned int pos) {
    openfiles[file].pos = pos;
    if( openfiles[file].pos > mixheaders[openfiles[file].id].size ) {
        openfiles[file].pos = mixheaders[openfiles[file].id].size;
    }
    mixfiles[mixheaders[openfiles[file].id].filenum]->seekSet(openfiles[file].pos+mixheaders[openfiles[file].id].offset);
}

void MIXFiles::seekCur(unsigned int file, int pos) {
    openfiles[file].pos += pos;
    if( openfiles[file].pos > mixheaders[openfiles[file].id].size ) {
        openfiles[file].pos = mixheaders[openfiles[file].id].size;
    }
    mixfiles[mixheaders[openfiles[file].id].filenum]->seekSet(openfiles[file].pos+mixheaders[openfiles[file].id].offset);
}


unsigned int MIXFiles::getPos(unsigned int file) const {
    /// @TODO Abstract this const version of operator[]
    std::map<unsigned int, MIXPriv::OpenFile>::const_iterator i = openfiles.find(file);
    if (openfiles.end() != i) {
        return i->second.pos;
    } else {
        /// @TODO Throw an exception in a later iteration of code cleanup.
        return 0;
    }
}

unsigned int MIXFiles::getSize(unsigned int file) const {
    /// @TODO Abstract this const version of operator[]
    openfiles_t::const_iterator i = openfiles.find(file);
    if (openfiles.end() != i) {
        mixheaders_t::const_iterator i2 = mixheaders.find(i->second.id);
        if (mixheaders.end() != i2) {
            return i2->second.size;
        }
    }
    /// @TODO Throw an exception in a later iterator of code cleanup.
    return 0;
}

const char* MIXFiles::getPath(unsigned int file) const {
    return NULL;
}

