#include <stdexcept>

#include "../lib/compression.h"
#include "../lib/inifile.h"
#include "../sound/sound_public.h"
#include "graphicsengine.h"
#include "imageproc.h"
#include "wsa.h"
#include "../lib/fcncendian.h"

using std::runtime_error;

namespace
{
    const int MAX_DECOMP_SIZE = 64000;

    vector<unsigned char> temp_buff(MAX_DECOMP_SIZE);
}


WSA::WSA(const string& fname)
{
    {
        shared_ptr<File> file = game.vfs.open(fname);
        if (!file) {
            throw WSAError("WSA: Unable to open '" + fname + "'");
        }
        wsadata.resize(file->size());
        if (file->read(wsadata, file->size()) == 0) {
            throw WSAError("WSA: Empty file (" + fname + ")");
        }
    }

    shared_ptr<INIFile> wsa_ini;
    try {
        wsa_ini = GetConfig("wsa.ini");
    } catch(runtime_error&) {
        throw WSAError("WSA: wsa.ini not found");
    }

    // FIXME: readString is going away later on
    char* tmp = wsa_ini->readString(fname.c_str(), "sound");
    if (tmp != NULL) {
        sndfile = tmp;
        delete[] tmp;
    }

    vector<unsigned char>::iterator it(wsadata.begin());
    c_frames = read_word(it, FCNC_LIL_ENDIAN);
    xpos = read_word(it, FCNC_LIL_ENDIAN);
    ypos = read_word(it, FCNC_LIL_ENDIAN);
    width = read_word(it, FCNC_LIL_ENDIAN);
    height = read_word(it, FCNC_LIL_ENDIAN);
    delta = read_dword(it, FCNC_LIL_ENDIAN);
    offsets.resize(c_frames + 2);

    for (int i = 0; i < c_frames + 2; ++i) {
        offsets[i] = read_dword(it, FCNC_LIL_ENDIAN) + 0x300;
    }

    for (int i = 0; i < 256; i++) {
        palette[i].r = read_byte(it);
        palette[i].g = read_byte(it);
        palette[i].b = read_byte(it);
        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
    }

    /* framedata contains the raw frame data
     * the first frame has to be decoded over a zeroed frame
     * and then each subsequent frame, decoded over the previous one
     */
    framedata.resize(height * width);

    if (offsets[c_frames + 1] - 0x300) {
        loop = true;
        // Is this correct?
        c_frames += 1; // Add loop frame
    } else {
        loop = false;
    }
}

SDL_Surface* WSA::decode_frame(unsigned short framenum)
{
    unsigned int len_frame = offsets[framenum+1] - offsets[framenum];
    vector<unsigned char> image80(len_frame);
    memcpy(&image80[0], &wsadata[offsets[framenum]], len_frame);

    Compression::decode80(&image80[0], &temp_buff[0]);
    Compression::decode40(&temp_buff[0], &framedata[0]);

    SDL_Surface* tempframe = SDL_CreateRGBSurfaceFrom(&framedata[0], width, height, 8, width, 0, 0, 0, 0);
    SDL_SetColors(tempframe, palette, 0, 256);

    return tempframe;
}

void WSA::animate()
{
    float fps, delay;
    SDL_Rect dest;
    int i;
    SDL_Surface *frame;
    ImageProc scaler;

    frame = NULL;
    dest.w = width<<1;
    dest.h = height<<1;
    dest.x = (pc::gfxeng->getWidth()-(width<<1))>>1;
    dest.y = (pc::gfxeng->getHeight()-(height<<1))>>1;
    fps = static_cast<float>((1024.0 / (float) delta) * 1024.0);
    delay = static_cast<float>((0.5 / fps) * 1000.0);

    pc::gfxeng->clearScreen();
    /* queue sound first, regardless of whats in the buffer already */
    if (c_frames == 0) {
        return;
    }
    int loopid;
    if (!sndfile.empty()) {
        loopid = pc::sfxeng->PlayLoopedSound(sndfile, 0);
    }
    for (i = 0; i < c_frames; i++) {
        /* FIXME: fill buffer a little before zero to prevent
         * slight audio pause? what value to use?
         */
        frame = decode_frame(i);
        pc::gfxeng->drawVQAFrame(scaler.scale(frame,1));
        SDL_Delay((unsigned int)delay);
    }
    SDL_FreeSurface(frame);
    if (!sndfile.empty()) {
        pc::sfxeng->StopLoopedSound(loopid);
    }
}
