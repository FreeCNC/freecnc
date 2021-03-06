#ifndef _SOUND_SOUNDCOMMON_H
#define _SOUND_SOUNDCOMMON_H

#include "../freecnc.h"

#define SOUND_FORMAT    AUDIO_S16SYS
#define SOUND_CHANNELS  2
#define SOUND_FREQUENCY 22050

#define SOUND_MAX_CHUNK_SIZE        16384
#define SOUND_MAX_UNCOMPRESSED_SIZE (SOUND_MAX_CHUNK_SIZE << 2)
#define SOUND_MAX_COMPRESSED_SIZE   SOUND_MAX_CHUNK_SIZE

typedef vector<unsigned char> SampleBuffer;
typedef vector<unsigned char>::iterator SampleIterator;

#endif /* SOUNDCOMMON_H */
