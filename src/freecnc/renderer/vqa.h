#ifndef _RENDERER_VQA_H
#define _RENDERER_VQA_H

#include <boost/noncopyable.hpp>
#include "SDL.h"
#include "../freecnc.h"

class VFile;

namespace VQAPriv {
// size of vqa header - so i know how many bytes to read in the file 
const size_t header_size = sizeof(Uint8) * 28 + sizeof(Uint16) * 9 + sizeof(Uint32) * 2;

struct VQAHeader
{
    Uint8 Signature[8]; // Always "WVQAVQHD"
    Uint32 RStartPos; // Size of header minus Signature (always 42 bytes) big endian
    Uint16 Version; // VQA Version. 2 = C&C TD and C&C RA, 3 = C&C TS
    Uint16 Flags;  // VQA Flags. If bit 1 is set, VQA has sound
    Uint16 NumFrames; // Number of frames
    Uint16 Width;  // Width of each frame
    Uint16 Height;  // Height of each frame
    Uint8 BlockW;  // Width of each image block (usually 4)
    Uint8 BlockH;  // Height of each image block (usually 2)
    Uint8 FrameRate; // Number of frames per second?
    Uint8 CBParts; // Number of frames that use the same lookup table (always 8 in TD and RA)
    Uint16 Colors;  // Number of colors used in Palette 
    Uint16 MaxBlocks; // Maximum number of image blocks??
    Uint16 Unknown1;
    Uint32 Unknown2;
    Uint16 Freq;  // Sound frequency
    Uint8 Channels; // 1 = mono; 2 = stereo (TD and RA always 1) (TS is always 2)
    Uint8 Bits;  // 8 or 16 bit sound
    Uint8 Unknown3[14];
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
    Uint32 DecodeSNDChunk(Uint8* outbuf);
    bool DecodeVQFRChunk(SDL_Surface* frame);
    inline void DecodeCBPChunk();
    inline void DecodeVPTChunk(Uint8 Compressed);
    inline void DecodeCBFChunk(Uint8 Compressed);
    inline void DecodeCPLChunk(SDL_Color* palette);
    inline void DecodeUnknownChunk();

    static void AudioHook(void* userdata, Uint8* stream, int len);

    // General VQA File Related variables 
    VFile* vqafile;
    VQAPriv::VQAHeader header;
    // VQA Video Related Variables 
    Uint32 CBPOffset;
    Uint16 CBPChunks;
    Uint8* CBF_LookUp;
    Uint8* CBP_LookUp;
    Uint8* VPT_Table;
    Uint32* offsets;
    Uint8 modifier;
    Uint32 lowoffset;
    // VQA Sound Related Variables 
    Sint32 sndindex;
    Sint32 sndsample;

    int scaleVideo, videoScaleQuality;

    // Buffer to hold ~15 audio frames 
    Uint8* sndbuf; // The whole buffer
    Uint8* sndbufMaxEnd; // The max end of the malloced size of the buffer
    Uint8* sndbufStart; // The current start into the buffer
    Uint8* sndbufEnd; // The current end in the buffer

    // Semaphores to allow sync 
    SDL_sem* empty;
    SDL_sem* full;
    SDL_mutex* sndBufLock;

    // Used to convert VQA audio to correct format 
    SDL_AudioCVT cvt;
};

#endif
