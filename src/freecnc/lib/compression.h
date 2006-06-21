#ifndef _LIB_COMPRESSION_H
#define _LIB_COMPRESSION_H

#include "../freecnc.h"

namespace Compression
{
    int decode80(const unsigned char image_in[], unsigned char image_out[]);
    int decode40(const unsigned char image_in[], unsigned char image_out[]);
    int decode20(const unsigned char* s, unsigned char* d, int cb_s);
    int dec_base64(const unsigned char* src, unsigned char* target, size_t length);
}

#endif
