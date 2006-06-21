#ifndef _RENDERER_VQA_H
#define _RENDERER_VQA_H

#include <boost/noncopyable.hpp>
#include "SDL.h"
#include "../freecnc.h"

class VFile;

namespace VQAPriv {
// size of vqa header - so i know how many bytes to read in the file 
const size_t header_size = sizeof(unsigned char) * 28 + sizeof(unsigned short) * 9 + sizeof(unsigned int) * 2;

struct VQAHeader
{
    unsigned char Signature[8]; // Always "WVQAVQHD"
    unsigned int RStartPos; // Size of header minus Signature (always 42 bytes) big endian
    unsigned short Version; // VQA Version. 2 = C&C TD and C&C RA, 3 = C&C TS
    unsigned short Flags;  // VQA Flags. If bit 1 is set, VQA has sound
    unsigned short NumFrames; // Number of frames
    unsigned short Width;  // Width of each frame
    unsigned short Height;  // Height of each frame
    unsigned char BlockW;  // Width of each image block (usually 4)
    unsigned char BlockH;  // Height of each image block (usually 2)
    unsigned char FrameRate; // Number of frames per second?
    unsigned char CBParts; // Number of frames that use the same lookup table (always 8 in TD and RA)
    unsigned short Colors;  // Number of colors used in Palette 
    unsigned short MaxBlocks; // Maximum number of image blocks??
    unsigned short Unknown1;
    unsigned int Unknown2;
    unsigned short Freq;  // Sound frequency
    unsigned char Channels; // 1 = mono; 2 = stereo (TD and RA always 1) (TS is always 2)
    unsigned char Bits;  // 8 or 16 bit sound
    unsigned char Unknown3[14];
};

}

class VFile;
class VQAMovie : boost::noncopyable
{
public:
    VQAMovie(const char* filename);
    ~VQAMovie();
    void play();
private:
    VQAMovie();

    bool DecodeFORMChunk(); // Decodes FORM Chunk - This has to return true to continue 
    bool DecodeFINFChunk(); // This has to return true to continue 
    unsigned int DecodeSNDChunk(unsigned char* outbuf);
    bool DecodeVQFRChunk(SDL_Surface* frame);
    inline void DecodeCBPChunk();
    inline void DecodeVPTChunk(unsigned char Compressed);
    inline void DecodeCBFChunk(unsigned char Compressed);
    inline void DecodeCPLChunk(SDL_Color* palette);
    inline void DecodeUnknownChunk();

    static void AudioHook(void* userdata, unsigned char* stream, int len);

    // General VQA File Related variables 
    VFile* vqafile;
    VQAPriv::VQAHeader header;
    // VQA Video Related Variables 
    unsigned int CBPOffset;
    unsigned short CBPChunks;
    unsigned char* CBF_LookUp;
    unsigned char* CBP_LookUp;
    unsigned char* VPT_Table;
    unsigned int* offsets;
    unsigned char modifier;
    unsigned int lowoffset;
    // VQA Sound Related Variables 
    int sndindex;
    int sndsample;

    int scaleVideo, videoScaleQuality;

    // Buffer to hold ~15 audio frames 
    unsigned char* sndbuf; // The whole buffer
    unsigned char* sndbufMaxEnd; // The max end of the malloced size of the buffer
    unsigned char* sndbufStart; // The current start into the buffer
    unsigned char* sndbufEnd; // The current end in the buffer

    // Semaphores to allow sync 
    SDL_sem* empty;
    SDL_sem* full;
    SDL_mutex* sndBufLock;

    // Used to convert VQA audio to correct format 
    SDL_AudioCVT cvt;
};

#endif
