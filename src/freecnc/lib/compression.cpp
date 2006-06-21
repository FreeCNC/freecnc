/*
    Decoding routines for format80, format40 and format20 graphics
    Based on decode.c by Olaf van der Spek.
*/

#include <algorithm>
#include <cstring>
#include <cctype>

#include "../freecnc.h"
#include "compression.h"

namespace Compression
{
    // Decompresses format80
    // image_in: Buffer of compressed data
    // image_out: Buffer to hold output
    // returns: size of uncompressed data
    int decode80(const unsigned char image_in[], unsigned char image_out[])
    {
        /*
        0 copy 0cccpppp p
        1 copy 10cccccc
        2 copy 11cccccc p p
        3 fill 11111110 c c v
        4 copy 11111111 c c p p
        */

        const unsigned char* copyp;
        const unsigned char* readp = image_in;
        unsigned char* writep = image_out;
        unsigned int code;
        unsigned int count;

        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        unsigned short bigend; /* temporary big endian var */
        #endif

        while (1) {
            code = *readp++;
            if (~code & 0x80) {
                //bit 7 = 0
                //command 0 (0cccpppp p): copy
                count = (code >> 4) + 3;
                copyp = writep - (((code & 0xf) << 8) + *readp++);
                while (count--)
                    *writep++ = *copyp++;
            } else {
                //bit 7 = 1
                count = code & 0x3f;
                if (~code & 0x40) {
                    //bit 6 = 0
                    if (!count)
                        //end of image
                        break;
                    //command 1 (10cccccc): copy
                    while (count--)
                        *writep++ = *readp++;
                } else {
                    //bit 6 = 1
                    if (count < 0x3e) {
                        //command 2 (11cccccc p p): copy
                        count += 3;
                        
                        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                        memcpy(&bigend, readp, 2);
                        copyp = &image_out[SDL_Swap16(bigend)];
                        #else
                        copyp = &image_out[*(unsigned short*)readp];
                        #endif

                        readp += 2;
                        while (count--)
                            *writep++ = *copyp++;
                    } else if (count == 0x3e) {
                        //command 3 (11111110 c c v): fill
                        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                        memset(&count, 0, sizeof(unsigned int));
                        memcpy(&count, readp, 2);
                        count = SDL_Swap32(count);
                        #else
                        count = *(unsigned short*)readp;
                        #endif

                        readp += 2;
                        code = *readp++;
                        while (count--)
                            *writep++ = code;
                    } else {
                        //command 4 (copy 11111111 c c p p): copy
                        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                        memset(&count, 0, sizeof(unsigned int));
                        memcpy(&count, readp, 2);
                        count = SDL_Swap32(count);
                        #else
                        count = *(unsigned short*)readp;
                        #endif

                        readp += 2;

                        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                        memcpy(&bigend, readp, 2);
                        copyp = &image_out[SDL_Swap16(bigend)];
                        #else
                        copyp = &image_out[*(unsigned short*)readp];
                        #endif

                        readp += 2;
                        while (count--)
                            *writep++ = *copyp++;
                    }
                }
            }
        }

        return (writep - image_out);
    }

    // Decompresses format40
    // image_in: Buffer of compressed data
    // image_out: Buffer to hold output
    // returns: size of uncompressed data
    int decode40(const unsigned char image_in[], unsigned char image_out[])
    {
        /*
        0 fill 00000000 c v
        1 copy 0ccccccc
        2 skip 10000000 c 0ccccccc
        3 copy 10000000 c 10cccccc
        4 fill 10000000 c 11cccccc v
        5 skip 1ccccccc
        */

        const unsigned char* readp = image_in;
        unsigned char* writep = image_out;
        unsigned int code;
        unsigned int count;

        while (1) {
            code = *readp++;
            if (~code & 0x80) {
                //bit 7 = 0
                if (!code) {
                    //command 0 (00000000 c v): fill
                    count = *readp++;
                    code = *readp++;
                    while (count--)
                        *writep++ ^= code;
                } else {
                    //command 1 (0ccccccc): copy
                    count = code;
                    while (count--)
                        *writep++ ^= *readp++;
                }

            } else {
                //bit 7 = 1
                if (!(count = code & 0x7f)) {
                    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
                    memset(&count, 0, sizeof(unsigned int));
                    memcpy(&count, readp, 2);
                    count = SDL_Swap32(count);
                    #else
                    count = *(unsigned short*)readp;
                    #endif

                    readp += 2;
                    code = count >> 8;
                    if (~code & 0x80) {
                        //bit 7 = 0
                        //command 2 (10000000 c 0ccccccc): skip
                        if (!count)
                            // end of image
                            break;
                        writep += count;
                    } else {
                        //bit 7 = 1
                        count &= 0x3fff;
                        if (~code & 0x40) {
                            //bit 6 = 0
                            //command 3 (10000000 c 10cccccc): copy
                            while (count--)
                                *writep++ ^= *readp++;
                        } else {
                            //bit 6 = 1
                            //command 4 (10000000 c 11cccccc v): fill
                            code = *readp++;
                            while (count--)
                                *writep++ ^= code;
                        }
                    }
                } else {
                    //command 5 (1ccccccc): skip
                    writep += count;
                }
            }
        }
        return (writep - image_out);
    }

    // Decompresses format20
    // s: Buffer of compressed data
    // d: Buffer to hold output
    // cb_s: size of compressed data?
    // returns: size of uncompressed data?
    int decode20(const unsigned char* s, unsigned char* d, int cb_s)
    {
        const unsigned char* r = s;
        const unsigned char* r_end = s + cb_s;
        unsigned char* w = d;
        while (r < r_end) {
            int v = *r++;
            if (v)
                *w++ = v;
            else {
                v = *r++;
                memset(w, 0, v);
                w += v;
            }
        }
        return w - d;

    }

    // Decodes Base64 data
    // src: Base64 data stream
    // target: Buffer to hold output
    // length: size of the Base64 data stream
    // returns: -1 if error
    int dec_base64(const unsigned char* src, unsigned char* target, size_t length)
    {
        int i;
        unsigned char a, b, c, d;
        static unsigned char dtable[256];
        int bits_to_skip = 0;

        for( i = length-1; src[i] == '='; i-- ) {
            bits_to_skip += 2;
            length--;
        }
        if( bits_to_skip >= 6 ) {
            logger->warning("Error in base64 (too many '=')\n");
            return -1;
        }

        for(i= 0;i<255;i++) {
            dtable[i]= 0x80;
        }
        for(i= 'A';i<='Z';i++) {
            dtable[i]= i-'A';
        }
        for(i= 'a';i<='z';i++) {
            dtable[i]= 26+(i-'a');
        }
        for(i= '0';i<='9';i++) {
            dtable[i]= 52+(i-'0');
        }
        dtable[(unsigned char)'+']= 62;
        dtable[(unsigned char)'/']= 63;
        dtable[(unsigned char)'=']= 0;


        while (length >= 4) {
            a = dtable[src[0]];
            b = dtable[src[1]];
            c = dtable[src[2]];
            d = dtable[src[3]];
            if( a == 0x80 || b == 0x80 ||
                    c == 0x80 || d == 0x80 ) {
                logger->warning("Illegal character\n");
            }
            target[0] = a << 2 | b >> 4;
            target[1] = b << 4 | c >> 2;
            target[2] = c << 6 | d;
            target+=3;
            length-=4;
            src += 4;
        }
        if( length > 0 ) {
            if( bits_to_skip == 4 && length == 2 ) {
                a = dtable[src[0]];
                b = dtable[src[1]];

                target[0] = a << 2 | b >> 4;
            } else if( bits_to_skip == 2 && length == 3 ) {
                a = dtable[src[0]];
                b = dtable[src[1]];
                c = dtable[src[2]];

                target[0] = a << 2 | b >> 4;
                target[1] = b << 4 | c >> 2;
            } else {
                logger->warning("Error in base64. #bits to skip doesn't match length\n");
                logger->warning("skip %d bits, %d chars left\n\"%s\"\n", bits_to_skip, (unsigned int)length, src);
                return -1;
            }
        }

        return 0;
    }
}
