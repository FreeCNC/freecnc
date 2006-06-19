#ifndef _LIB_COMPRESSION_H
#define _LIB_COMPRESSION_H

#include "../freecnc.h"

namespace Compression
{
    int decode80(const Uint8 image_in[], Uint8 image_out[]);
    int decode40(const Uint8 image_in[], Uint8 image_out[]);
    int decode20(const Uint8* s, Uint8* d, int cb_s);
    int dec_base64(const Uint8* src, Uint8* target, size_t length);
}

#endif
