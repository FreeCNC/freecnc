#ifndef _SOUND_SOUNDFILE_H
#define _SOUND_SOUNDFILE_H

#include "../freecnc.h"
#include "soundcommon.h"

struct SDL_AudioCVT;
class VFile;

enum {
    SOUND_DECODE_ERROR = 0,
    SOUND_DECODE_COMPLETED = 1,
    SOUND_DECODE_STREAMING = 2
};

namespace Sound {
    void IMADecode(unsigned char *output, unsigned char *input, unsigned short compressed_size, int& sample, int& index);
    void WSADPCM_Decode(unsigned char *output, unsigned char *input, unsigned short compressed_size, unsigned short uncompressed_size);
}

class SoundFile
{
public:
    SoundFile();
    ~SoundFile();
    
    bool Open(const string& filename);
    void Close();
   
    // Length is the max size in bytes of the uncompressed sample, returned
    // in buffer. If length is zero, the full file is decoded.
    unsigned int Decode(SampleBuffer& buffer, unsigned int length = 0);

private:
    // File data
    std::string filename;
    VFile* file;
    //unsigned int offset;
    bool fileOpened;
    
    // Header information
    unsigned short frequency;
    unsigned int comp_size, uncomp_size;
    unsigned char flags, type;
    
    // IMADecode state
    int imaSample;
    int imaIndex;

    SDL_AudioCVT* conv;
};

#endif
