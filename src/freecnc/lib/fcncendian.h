#ifndef _LIB_FCNCENDIAN_H
#define _LIB_FCNCENDIAN_H
//
// Defines various useful constants and inline functions for endianness. 
//

#include <cstdio>
#include "SDL_endian.h"

#define FCNC_LIL_ENDIAN 1234
#define FCNC_BIG_ENDIAN 4321

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define FCNC_BYTEORDER FCNC_LIL_ENDIAN
#else
#define FCNC_BYTEORDER FCNC_BIG_ENDIAN
#endif

//-----------------------------------------------------------------------------
   
// Swaps `word' endianness.
inline unsigned short swap_endian(unsigned short word)
{
    return (word << 8) | (word >> 8);
}

// Swaps `dword' endianness.
inline unsigned int swap_endian(unsigned int dword)
{
    return (dword << 24) | ((dword << 8) & 0x00FF0000) | ((dword >> 8) & 0x0000FF00) | (dword >> 24);
}

//-----------------------------------------------------------------------------

// Swaps `word' to system endianess from `byteorder'.
// No swapping is performed on a system with the same endianness as `byteorder', or if byteorder is 0.
inline unsigned short swap_sys_endian(unsigned short word, int byteorder)
{
    if (byteorder != 0 && byteorder != FCNC_BYTEORDER) {
        word = swap_endian(word);
    }
    return word;
}

// Swaps `dword' to system endianess from `byteorder'.
// No swapping is performed on a system with the same endianness as `byteorder', or if byteorder is 0.
inline unsigned int swap_sys_endian(unsigned int dword, int byteorder)
{
    if (byteorder != 0 && byteorder != FCNC_BYTEORDER) {
        dword = swap_endian(dword);
    }
    return dword;
}

//-----------------------------------------------------------------------------

// Swaps `word' to litte-endian from system endianness.
// No swapping is performed on a little-endian system.
inline unsigned short swap_little_endian(unsigned short word)
{
    #if FCNC_BYTEORDER == FCNC_LIL_ENDIAN
    return word;
    #else 
    return swap_endian(word);
    #endif
}

// Swaps `dword' to litte-endian from system endianness.
// No swapping is performed on a little-endian system.
inline unsigned int swap_little_endian(unsigned int dword)
{
    #if FCNC_BYTEORDER == FCNC_LIL_ENDIAN
    return dword;
    #else 
    return swap_endian(dword);
    #endif
}

//-----------------------------------------------------------------------------

// Swaps `word' to big-endian from system endianness.
// No swapping is performed on a big-endian system.
inline unsigned short swap_big_endian(unsigned short word)
{
    #if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
    return word;
    #else 
    return swap_endian(word);
    #endif
}

// Swaps `dword' to big-endian from system endianness.
// No swapping is performed on a big-endian system.
inline unsigned int swap_big_endian(unsigned int dword)
{
    #if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
    return dword;
    #else 
    return swap_endian(dword);
    #endif
}

//-----------------------------------------------------------------------------

// Reads a byte from `it' and increases the iterator.
// The iterator is assumed to dereference contiguous bytes. It will be increased once.
template<class Iterator>
inline unsigned char read_byte(Iterator& it)
{
    unsigned char byte = *it;
    ++it;
    return byte;
}
    
// Reads a word from `it', swapping to system endianness from `byteorder'.
// The iterator is assumed to dereference contiguous bytes. It will be increased twice.
// No swapping is performed on a system with the same endianness as `byteorder', or if byteorder is 0.
template<class Iterator>
inline unsigned short read_word(Iterator& it, int byteorder=0)
{
    unsigned short word = swap_sys_endian(*reinterpret_cast<unsigned short*>(&*it), byteorder);
    ++it;
    ++it;
    return word;
}
    
// Reads a dword from `it', swapping to system endianness from `byteorder'.
// The iterator is assumed to dereference contiguous bytes. It will be increased 4 times.
// No swapping is performed on a system with the same endianness as `byteorder', or if byteorder is 0.
template<class Iterator>
inline unsigned int read_dword(Iterator& it, int byteorder=0)
{
    unsigned short dword = swap_sys_endian(*reinterpret_cast<unsigned int*>(&*it), byteorder);
    ++it;
    ++it;
    ++it;
    ++it;
    return dword;
}

//-----------------------------------------------------------------------------
// Old stuff that needs to go
//-----------------------------------------------------------------------------

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define readbyte(x,y) x[y]
#define readword(x,y) x[y] + (x[y+1] << 8)
#define readthree(x,y)  x[y] + (x[y+1] << 8) + (x[y+2] << 16) + (0 << 24)
#define readlong(x,y) x[y] + (x[y+1] << 8) + (x[y+2] << 16) + (x[y+3] << 24)
#else
#define readbyte(x,y) x[y]
#define readword(x,y) SDL_Swap16((x[y] << 8) ^ x[y+1])
#define readthree(x,y) SDL_Swap32((x[y] << 24) ^ (x[y+1] << 16) ^ (x[y+2] << 8))
#define readlong(x,y) SDL_Swap32((x[y] << 24) ^ (x[y+1] << 16) ^ (x[y+2] << 8) ^ (x[y+3]))
#endif

inline unsigned char freadbyte(FILE *fptr)
{
    unsigned char x;
    fread(&x,1,1,fptr);
    return x;
}

inline unsigned short freadword(FILE *fptr)
{
    unsigned short x;
    fread(&x,2,1,fptr);

    #if FCNC_BYTEORDER == FCNC_LIL_ENDIAN
    return x;
    #else
    return SDL_Swap16(x);
    #endif
}

inline unsigned int freadthree(FILE *fptr)
{
    // Can this be made better?
    unsigned char x[3];
    fread(x,3,1,fptr);
    return readthree(x,0);
}

inline unsigned int freadlong(FILE *fptr)
{
    unsigned int x;
    fread(&x, 4, 1, fptr);

    #if FCNC_BYTEORDER == FCNC_LIL_ENDIAN
    return x;
    #else
    return SDL_Swap32(x);
    #endif
}

#endif
