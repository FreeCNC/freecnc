#ifndef _LIB_FCNCENDIAN_H
#define _LIB_FCNCENDIAN_H
//
// Defines various useful constants and inline functions for endianness. 
//

#include <iterator>
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

// Swaps `dword' to/from little endian.
// No swapping is performed on a little endian system.
inline unsigned short little_endian(unsigned short word)
{
    #if FCNC_BYTEORDER == FCNC_LIL_ENDIAN
    return word;
    #else 
    return swap_endian(word);
    #endif
}

// Swaps `dword' to/from little endian.
// No swapping is performed on a little endian system.
inline unsigned int little_endian(unsigned int dword)
{
    #if FCNC_BYTEORDER == FCNC_LIL_ENDIAN
    return dword;
    #else 
    return swap_endian(dword);
    #endif
}

//-----------------------------------------------------------------------------

// Swaps `word' to/from big endian.
// No swapping is performed on a big endian system.
inline unsigned short big_endian(unsigned short word)
{
    #if FCNC_BYTEORDER == FCNC_BIG_ENDIAN
    return word;
    #else 
    return swap_endian(word);
    #endif
}

// Swaps `dword' to/from big endian.
// No swapping is performed on a big endian system.
inline unsigned int big_endian(unsigned int dword)
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
    unsigned char byte = *reinterpret_cast<unsigned char*>(&*it);
    ++it;
    return byte;
}
    
// Reads a word from `it', converting to system endianness from `byteorder'.
// The iterator is assumed to dereference contiguous bytes. It will be increased twice.
// No swapping is performed on a system with the same endianness as `byteorder', or if byteorder is 0.
template<class Iterator>
inline unsigned short read_word(Iterator& it, int byteorder=0)
{
    unsigned short word = *reinterpret_cast<unsigned short*>(&*it);
    if (byteorder != 0) {
        word = byteorder == FCNC_LIL_ENDIAN ? little_endian(word) : big_endian(word);
    }
    std::advance(it, 2);
    return word;
}
    
// Reads a dword from `it', converting to system endianness from `byteorder'.
// The iterator is assumed to dereference contiguous bytes. It will be increased 4 times.
// No swapping is performed on a system with the same endianness as `byteorder', or if byteorder is 0.
template<class Iterator>
inline unsigned int read_dword(Iterator& it, int byteorder=0)
{
    unsigned int dword = *reinterpret_cast<unsigned int*>(&*it);
    if (byteorder != 0) {
        dword = byteorder == FCNC_LIL_ENDIAN ? little_endian(dword) : big_endian(dword);
    }
    std::advance(it, 4);
    return dword;
}

//-----------------------------------------------------------------------------

// Reads a byte from `ptr' and returns it.
// It is assumed `ptr' points to at least 1 byte of data.
inline unsigned char read_byte(void* ptr)
{
    unsigned char byte = *reinterpret_cast<unsigned char*>(ptr);
    return byte;
}

// Reads a word from `ptr', converting to system endianness from `byteorder'.
// It is assumed `ptr' points to at least 2 bytes of data.
// No swapping is performed on a system with the same endianness as `byteorder', or if byteorder is 0.
inline unsigned short read_word(void* ptr, int byteorder=0)
{
    unsigned short word = *reinterpret_cast<unsigned short*>(ptr);
    if (byteorder != 0) {
        word = byteorder == FCNC_LIL_ENDIAN ? little_endian(word) : big_endian(word);
    }
    return word;
}

// Reads a dword from `ptr', converting to system endianness from `byteorder'.
// It is assumed `ptr' points to at least 4 bytes of data.
// No swapping is performed on a system with the same endianness as `byteorder', or if byteorder is 0.
inline unsigned int read_dword(void* ptr, int byteorder=0)
{
    unsigned int dword = *reinterpret_cast<unsigned int*>(ptr);
    if (byteorder != 0) {
        dword = byteorder == FCNC_LIL_ENDIAN ? little_endian(dword) : big_endian(dword);
    }
    return dword;
}

#endif
