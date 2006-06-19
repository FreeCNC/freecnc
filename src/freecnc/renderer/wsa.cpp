#include <stdexcept>

#include "../lib/compression.h"
#include "../lib/inifile.h"
#include "../sound/sound_public.h"
#include "../vfs/vfs_public.h"
#include "graphicsengine.h"
#include "imageproc.h"
#include "wsa.h"

using std::runtime_error;

WSA::WSA(const char* fname)
{
    int i, j;
    shared_ptr<INIFile> wsa_ini;
    VFile* wfile;

    /* Extract entire animation */
    wfile = VFS_Open(fname);
    if( wfile == NULL ) {
        throw WSAError();
    }
    wsadata = new Uint8[wfile->fileSize()];
    wfile->readByte(wsadata, wfile->fileSize());
    VFS_Close(wfile);
    //wsadata = mixes->extract(fname);

    /* Read which sound needs to be played */
    try {
        wsa_ini = GetConfig("wsa.ini");
    } catch(runtime_error&) {
        logger->error("wsa.ini not found.\n");
        throw WSAError();
    }

    sndfile = wsa_ini->readString(fname, "sound");

    if (wsadata != NULL) {
        /* Lets get the header */
        header.NumFrames = readword(wsadata, 0);
        header.xpos = readword(wsadata, 2);
        header.ypos = readword(wsadata, 4);
        header.width = readword(wsadata, 6);
        header.height = readword(wsadata, 8);
        header.delta = readlong(wsadata, 10);
        header.offsets = new Uint32[header.NumFrames + 2];
        memset(header.offsets, 0, (header.NumFrames + 2) * sizeof(Uint32));
        j = 14; /* start of offsets */
        for (i = 0; i < header.NumFrames + 2; i++) {
            header.offsets[i] = readlong(wsadata, j) + 0x300;
            j += 4;
        }

        /* Read the palette */
        for (i = 0; i < 256; i++) {
            palette[i].r = readbyte(wsadata, j);
            palette[i].g = readbyte(wsadata, j+1);
            palette[i].b = readbyte(wsadata, j+2);
            palette[i].r <<= 2;
            palette[i].g <<= 2;
            palette[i].b <<= 2;
            j += 3;
        }

        /* framedata contains the raw frame data
         * the first frame has to be decoded over a zeroed frame
         * and then each subsequent frame, decoded over the previous one
         */
        framedata = new Uint8[header.height * header.width];
        memset(framedata, 0, header.height * header.width);

        if (header.offsets[header.NumFrames + 1] - 0x300) {
            loop = 1;
            header.NumFrames += 1; /* Add loop frame */
        } else {
            loop = 0;
        }
    } else {
        throw WSAError();
    }

}

WSA::~WSA()
{
    delete[] wsadata;
    delete[] header.offsets;
    delete[] framedata;
    delete[] sndfile;
}

SDL_Surface* WSA::decodeFrame(Uint16 framenum)
{
    Uint32 FrameLen;
    Uint8 *image40, *image80;
    SDL_Surface *tempframe;
    // SDL_Surface *frame;

    /* Get length of specific frame */
    FrameLen = header.offsets[framenum+1] - header.offsets[framenum];
    image80 = new Uint8[FrameLen];
    image40 = new Uint8[64000]; /* Max space. We dont know how big
                                               decompressed image will be */

    memcpy(image80, wsadata + header.offsets[framenum], FrameLen);
    Compression::decode80(image80, image40);
    Compression::decode40(image40, framedata);

    tempframe = SDL_CreateRGBSurfaceFrom(framedata, header.width, header.height, 8, header.width, 0, 0, 0, 0);
    SDL_SetColors(tempframe, palette, 0, 256);

    // frame = SDL_DisplayFormat(tempframe);

    delete[] image40;
    delete[] image80;
    //SDL_FreeSurface(tempframe);

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
    dest.w = header.width<<1;
    dest.h = header.height<<1;
    dest.x = (pc::gfxeng->getWidth()-(header.width<<1))>>1;
    dest.y = (pc::gfxeng->getHeight()-(header.height<<1))>>1;
    fps = static_cast<float>((1024.0 / (float) header.delta) * 1024.0);
    delay = static_cast<float>((1.0 / fps) * 1000.0);

    pc::gfxeng->clearScreen();
    /* queue sound first, regardless of whats in the buffer already */
    if (header.NumFrames == 0) {
        return;
    }
    if (sndfile != NULL)
        pc::sfxeng->PlaySound(sndfile);
    for (i = 0; i < header.NumFrames; i++) {
        /* FIXME: fill buffer a little before zero to prevent
         * slight audio pause? what value to use?
         */
//        if (pc::sfxeng->getBufferLen() == 0 && sndfile != NULL)
            pc::sfxeng->PlaySound(sndfile);
        frame = decodeFrame(i);
        pc::gfxeng->drawVQAFrame(scaler.scale(frame,1));
        SDL_Delay((unsigned int)delay);
    }
    SDL_FreeSurface(frame);

    /* Perhaps an emptyBuffer function to empty any left over sound
     * in the buffer after the wsa finishes?
     */
}
