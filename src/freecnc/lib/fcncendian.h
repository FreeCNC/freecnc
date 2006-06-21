#ifndef _LIB_ENDIANUTILS_H
#define _LIB_ENDIANUTILS_H
//
// Defines various useful constants for endianness. 
//
// TODO: Replace macros with inline functions

#include <cstdio>
#include "SDL_endian.h"

#define FCNC_LIL_ENDIAN 1234
#define FCNC_BIG_ENDIAN 4321

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define FCNC_BYTEORDER FCNC_LIL_ENDIAN
#define readbyte(x,y) x[y]
#define readword(x,y) x[y] + (x[y+1] << 8)
#define readthree(x,y)  x[y] + (x[y+1] << 8) + (x[y+2] << 16) + (0 << 24)
#define readlong(x,y) x[y] + (x[y+1] << 8) + (x[y+2] << 16) + (x[y+3] << 24)
#else
#define FCNC_BYTEORDER FCNC_BIG_ENDIAN
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
