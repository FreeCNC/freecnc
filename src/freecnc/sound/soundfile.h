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
    void IMADecode(Uint8 *output, Uint8 *input, Uint16 compressed_size, Sint32& sample, Sint32& index);
    void WSADPCM_Decode(Uint8 *output, Uint8 *input, Uint16 compressed_size, Uint16 uncompressed_size);
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
    Uint32 Decode(SampleBuffer& buffer, Uint32 length = 0);

private:
    // File data
    std::string filename;
    VFile* file;
    //Uint32 offset;
    bool fileOpened;
    
    // Header information
    Uint16 frequency;
    Uint32 comp_size, uncomp_size;
    Uint8 flags, type;
    
    // IMADecode state
    Sint32 imaSample;
    Sint32 imaIndex;

    SDL_AudioCVT* conv;
};

#endif
