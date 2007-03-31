#ifndef _RENDERER_VQA_H
#define _RENDERER_VQA_H

#include <boost/noncopyable.hpp>
#include "SDL.h"
#include "../freecnc.h"
#include "../sound/sound_public.h"

class File;

namespace VQAPriv
{
    // Not sizeof(VQAHeader) because of alignment
    const unsigned int header_size = 54;

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

struct VQAError : public std::runtime_error
{
    VQAError(const string& msg) : std::runtime_error(msg) {}
};

class VQAMovie : boost::noncopyable
{
public:
    VQAMovie(const string& filename);
    ~VQAMovie();
    void play();
private:
    VQAMovie();

    bool DecodeFORMChunk();
    bool DecodeFINFChunk();
    unsigned int DecodeSNDChunk(unsigned char* outbuf);
    bool DecodeVQFRChunk(SDL_Surface* frame);
    inline void DecodeCBPChunk();
    inline void DecodeVPTChunk(bool compressed);
    inline void DecodeCBFChunk(bool compressed);
    inline void DecodeCPLChunk(SDL_Color* palette);
    inline void DecodeUnknownChunk();

    static void AudioHook(void* userdata, unsigned char* stream, int len);

    // VQA Video Related Variables 
    vector<unsigned char> CBF_LookUp;
    vector<unsigned char> CBP_LookUp;
    unsigned int CBPOffset;
    unsigned short CBPChunks;

    shared_ptr<File> vqafile;
    VQAPriv::VQAHeader header;

    vector<unsigned char> VPT_Table;
    vector<unsigned int> offsets;
    unsigned char modifier;
    unsigned int lowoffset;

    // VQA Sound Related Variables 
    int sndindex;
    int sndsample;

    int scaleVideo, videoScaleQuality;

    // Buffer to hold ~15 audio frames 
    SampleBuffer sndbuf; // The whole buffer
    size_t sndbufMaxEnd; // The max end of the malloced size of the buffer
    size_t sndbufStart; // The current start into the buffer
    size_t sndbufEnd; // The current end in the buffer

    // Semaphores to allow sync 
    SDL_sem* empty;
    SDL_sem* full;
    SDL_mutex* sndBufLock;

    // Used to convert VQA audio to correct format 
    SDL_AudioCVT cvt;
};

#endif
