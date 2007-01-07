#include <cassert>
#include <cctype>
#include <iomanip>
#include <stdexcept>

#include "../lib/compression.h"
#include "../sound/sound_public.h"
#include "../legacyvfs/vfs_public.h"
#include "graphicsengine.h"
#include "imageproc.h"
#include "vqa.h"
#include "../lib/fcncendian.h"

using std::runtime_error;

namespace VQAPriv
{
// Copied from XCC Mixer (xcc.ra2.mods) by Olaf van der Spek

/// @TODO Fix/verify this for Big Endian

inline unsigned char VQA_HI_BYTE(unsigned int x) {
    return (x & 0xff000000) >> 24;
}

const unsigned int vqa_t_mask = 0x00ffffff;

// Each of these constants is derived from e.g. *(unsigned int*)"CBF\0" etc.
// We can't have these in the above form if we want to use them as switch
// labels, even though they're consts.....
const unsigned int vqa_cbf_id = 0x464243;
const unsigned int vqa_cbp_id = 0x504243;
const unsigned int vqa_cpl_id = 0x4C5043;
const unsigned int vqa_snd_id = 0x444E53;
const unsigned int vqa_vpt_id = 0x545056;
const unsigned int vqa_vpr_id = 0x525056;
const unsigned int vqa_vqfl_id = 0x4C465156;
const unsigned int vqa_vqfr_id = 0x52465156;
}

using namespace VQAPriv;

/// @TODO We really need a glossary for all the VQA jargon
// VQA: Vector Quantised Animation
// SND: Sound
// FINF: Offsets (for what?) Info
// FORM: Contains header data
// CBP:
// CBF:
// CPL: Colour Palette (?)
// VPT: ??? - Last chunk in a frame
// VQFR: VQ Frame (?)

/**
 * @param the name of the vqamovie.
 */
VQAMovie::VQAMovie(const char* filename) : vqafile(0), CBF_LookUp(0),
        CBP_LookUp(0), VPT_Table(0), offsets(0), sndbuf(0)
{
    string fname(filename);

    if (toupper(filename[0]) != 'X') {
        fname += ".VQA";
        vqafile = VFS_Open(fname.c_str());
    }

    if (vqafile == 0) {
        throw runtime_error("No VQA file");
    }
    // Get header information for the vqa.  If the header is corrupt, we can die now.
    vqafile->seekSet(0);
    if (!DecodeFORMChunk()) {
        game.log << "VQA: Invalid FORM chunk" << endl;
        throw runtime_error("VQA: Invalid FORM chunk\n");
    }

    // DecodeFINFChunk uses offsets
    offsets = new unsigned int[header.NumFrames];
    if (!DecodeFINFChunk()) {
        delete[] offsets;
        game.log << "VQA: Invalid FINF chunk" << endl;
        throw runtime_error("VQA: Invalid FINF chunk\n");
    }

    CBF_LookUp = new unsigned char[0x0ff00 << 3];
    CBP_LookUp = new unsigned char[0x0ff00 << 3];
    VPT_Table = new unsigned char[lowoffset<<1];
    CBPOffset = 0; // Starting offset of CBP Look up table must be zero
    CBPChunks = 0; // Number of CBPChunks 

    scaleVideo = game.config.scale_movies;
    videoScaleQuality = game.config.scaler_quality;

    //logger->debug("Video is %dfps %d %d %d\n", header.FrameRate, header.Freq, header.Channels, header.Bits);

    if (SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, header.Channels, header.Freq, SOUND_FORMAT, SOUND_CHANNELS, SOUND_FREQUENCY) < 0) {
        game.log << "Could not build SDL_BuildAudioCVT filter" << endl;
        return;
    }

    empty = SDL_CreateSemaphore(0);
    full = SDL_CreateSemaphore(1);
    sndBufLock = SDL_CreateMutex();

    sndbuf = new unsigned char[2 * SOUND_MAX_UNCOMPRESSED_SIZE];
    sndbufMaxEnd = sndbuf + 2 * SOUND_MAX_UNCOMPRESSED_SIZE;
    sndbufStart = sndbuf;
    sndbufEnd = sndbuf;
}

VQAMovie::~VQAMovie()
{
    delete[] CBF_LookUp;
    delete[] CBP_LookUp;
    delete[] VPT_Table;
    delete[] offsets;
    VFS_Close(vqafile);

    SDL_DestroySemaphore(empty);
    SDL_DestroySemaphore(full);
    SDL_DestroyMutex(sndBufLock);

    delete[] sndbuf;
}

/// Helper function for SDL_mixer to play VQA sound and synchronise
void VQAMovie::AudioHook(void* udata, unsigned char* stream, int len)
{
    VQAMovie* vqa = (VQAMovie* )udata;

    // We only need half of what they ask for, since SDL_ConvertAudio 
    // makes our audio bigger
    len = len / vqa->cvt.len_mult;

    // While the buffer isn't filled, wait
    SDL_LockMutex(vqa->sndBufLock);
    while ((vqa->sndbufEnd - vqa->sndbufStart) < len) {
        SDL_UnlockMutex(vqa->sndBufLock);
        SDL_SemWait(vqa->empty);
        SDL_LockMutex(vqa->sndBufLock);
    }

    // Copy our buffered data into the stream
    memcpy(stream, vqa->sndbufStart, len);

    // Convert the audio to the format we like
    vqa->cvt.buf = stream;
    vqa->cvt.len = len;

    if (SDL_ConvertAudio(&vqa->cvt) < 0) {
        game.log << "Could not run conversion filter: " << SDL_GetError() << endl;
        return;
    }

    // Move the start up
    vqa->sndbufStart += len;

    // If we are too near the end, move back some
    if ((vqa->sndbufEnd + SOUND_MAX_UNCOMPRESSED_SIZE) >= vqa->sndbufMaxEnd) {

        int len = (int)(vqa->sndbufEnd - vqa->sndbufStart);

        // Move remaining data down
        memmove(vqa->sndbuf, vqa->sndbufStart, len);

        // Reset the start & ends
        vqa->sndbufStart = vqa->sndbuf;
        vqa->sndbufEnd = vqa->sndbufStart + len;
    }

    SDL_SemPost(vqa->full);

    SDL_UnlockMutex(vqa->sndBufLock);
}

/// Play the vqamovie
void VQAMovie::play()
{
    if (vqafile == 0)
        return;

    if (pc::sfxeng->NoSound()) {
        return;
    }
    SDL_Surface *frame, *cframe;
    SDL_Rect dest;
    SDL_Event esc_event;
    static ImageProc scaler;

    dest.w = header.Width<<1;
    dest.h = header.Height<<1;
    dest.x = (pc::gfxeng->getWidth()-(header.Width<<1))>>1;
    dest.y = (pc::gfxeng->getHeight()-(header.Height<<1))>>1;

    pc::gfxeng->clearScreen();

    // Seek to first frame/snd information of vqa
    vqafile->seekSet(offsets[0]);

    sndindex = 0;
    sndsample = 0;

    // create the frame to store the image in. 
    frame = SDL_CreateRGBSurface(SDL_SWSURFACE, header.Width, header.Height, 8, 0, 0, 0, 0);

    // Initialise the scaler 
    if (scaleVideo)
        scaler.initVideoScale(frame, videoScaleQuality);

    unsigned int jumplen;
    unsigned int sndlen = DecodeSNDChunk(sndbufEnd);
    sndbufEnd += sndlen;

    // The jump len is how far ahead we are allowed to be
    jumplen = sndlen;

    // Start music (aka the sound)
    pc::sfxeng->SetMusicHook(AudioHook, this);
    for (unsigned int framenum = 0; framenum < header.NumFrames; ++framenum) {
        SDL_LockMutex(sndBufLock);

        // Decode SND Chunk first 
        sndlen = DecodeSNDChunk(sndbufEnd);
        sndbufEnd += sndlen;

        // Signal we have more audio data
        SDL_SemPost(empty);

        // Wait until the buffer has been emptied again
        while ((sndbufEnd - sndbufStart) >= (int)(jumplen - (2 * sndlen))) {
            SDL_UnlockMutex(sndBufLock);
            SDL_SemWait(full);
            SDL_LockMutex(sndBufLock);
        }

        SDL_UnlockMutex(sndBufLock);

        if (!DecodeVQFRChunk(frame)) {
            game.log << "VQA: Decoding VQA frame" << endl;
            break;
        }

        if (scaleVideo) {
            cframe = scaler.scaleVideo(frame);
            pc::gfxeng->drawVQAFrame(cframe);
        } else {
            pc::gfxeng->drawVQAFrame(frame);
        }

        while (SDL_PollEvent(&esc_event)) {
            if (esc_event.type == SDL_KEYDOWN) {
                if (esc_event.key.state != SDL_PRESSED) {
                    break;
                }
                if (esc_event.key.keysym.sym == SDLK_ESCAPE) {
                    framenum = header.NumFrames; // Jump to end of video
                    break;
                }
            }
        }
    }

    pc::sfxeng->SetMusicHook(0, 0);

    SDL_FreeSurface(frame);
    if (scaleVideo) {
        scaler.closeVideoScale();
    }
}

bool VQAMovie::DecodeFORMChunk()
{
    char chunkid[5];

    vqafile->readByte((unsigned char*)chunkid, 4);
    chunkid[4] = 0;

    if (strncmp(chunkid, "FORM", 4)) {
        game.log << "VQA: Decoding FORM Chunk - Expected \"FORM\", got \"" << chunkid <<"\""
                 << endl;
        return false;
    }

    // skip chunklen 
    vqafile->seekCur(4);
#if FCNC_BYTEORDER == FCNC_LIL_ENDIAN
    vqafile->readByte(reinterpret_cast<unsigned char *>(&header), header_size);
#else
    vqafile->readByte((unsigned char*)&header.Signature, sizeof(header.Signature));
    vqafile->readDWord(&header.RStartPos, 1);
    vqafile->readWord(&header.Version, 1);
    vqafile->readWord(&header.Flags, 1);
    vqafile->readWord(&header.NumFrames, 1);
    vqafile->readWord(&header.Width, 1);
    vqafile->readWord(&header.Height, 1);
    vqafile->readByte(&header.BlockW, 1);
    vqafile->readByte(&header.BlockH, 1);
    vqafile->readByte(&header.FrameRate, 1);
    vqafile->readByte(&header.CBParts, 1);
    vqafile->readWord(&header.Colors, 1);
    vqafile->readWord(&header.MaxBlocks, 1);
    vqafile->readWord(&header.Unknown1, 1);
    vqafile->readDWord(&header.Unknown2, 1);
    vqafile->readWord(&header.Freq, 1);
    vqafile->readByte(&header.Channels, 1);
    vqafile->readByte(&header.Bits, 1);
    vqafile->readByte((unsigned char*)&header.Unknown3, sizeof(header.Unknown3));
#endif
    // Weird: need to byteswap on both BE and LE
    // readDWord probably swaps back on BE.
    header.RStartPos = SDL_Swap32(header.RStartPos);
    if (strncmp((const char*)header.Signature, "WVQAVQHD", 8) == 1) {
        string sig((const char*)header.Signature, 8);
        game.log << "VQA: Invalid header: Expected \"WVQAVQHD\", got \""
                 << sig << "\"" << endl;
        return false;
    }
    if (header.Version != 2) {
        game.log << "VQA: Unsupported version: Expected 2, got " << header.Version << endl;
        return false;
    }
    // Set some constants based on the header
    lowoffset = (header.Width/header.BlockW)*(header.Height/header.BlockH);
    modifier = header.BlockH == 2 ? 0x0f : 0xff;
    return true;
}

bool VQAMovie::DecodeFINFChunk()
{
    unsigned char chunkid[5];

    vqafile->readByte(chunkid, 4);
    chunkid[4] = 0;

    if (strncmp((const char*)chunkid, "FINF", 4)) {
        game.log << "VQA: Decoding FINF chunk - Expected \"FINF\", got \"" << chunkid << "\""
                 << endl;
        return false;
    }

    unsigned int chunklen;
    vqafile->readDWord(&chunklen, 1);
    chunklen = SDL_Swap32(chunklen);
    if (static_cast<unsigned int>(header.NumFrames << 2) != chunklen) {
        game.log << "VQA: Invalid chunk length (" << (header.NumFrames << 2) 
                 << " != " << chunklen << endl;
        return false;
    }

    vqafile->readByte((unsigned char*)offsets, header.NumFrames<<2);
    unsigned int* offp = offsets;
    for (unsigned int framenum = 0; framenum < header.NumFrames; ++framenum, ++offp) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        *offp = SDL_Swap32(*offp);
#endif
        *offp &= 0x3FFFFFFF;
        *offp <<= 1;
    }

    return true;
}

/** Decodes SND Chunk
 * @param pointer to store the decoded chunk
 * @return length of chunk
 */
unsigned int VQAMovie::DecodeSNDChunk(unsigned char *outbuf)
{
    unsigned int chunklen;
    unsigned int chunkid;
    unsigned char *inbuf;

    // header Flags tells us that this VQA does not support sounds.. so lets quit
    if (!(header.Flags & 1))
        return 0;

    // seek to correct offset
    if (vqafile->tell() & 1)
        vqafile->seekCur(1);

    vqafile->readDWord(&chunkid, 1);
    if ((chunkid & vqa_t_mask) != vqa_snd_id) {
        game.log << "VQA: Decoding SND chunk - Expected 0x"
                 << std::hex << vqa_snd_id << ", got 0x" << (chunkid & vqa_t_mask)
                 << std::dec << endl;
        return 0; // Returning zero here, to set length of sound chunk to zero
    }

    vqafile->readDWord(&chunklen, 1);
    chunklen = SDL_Swap32(chunklen);

    inbuf = new unsigned char[chunklen];

    vqafile->readByte(inbuf, chunklen);

    switch (VQA_HI_BYTE(chunkid)) {
    case '0': // Raw uncompressed wave data 
        memcpy(outbuf, inbuf, chunklen);
        break;
    case '1': // Westwoods own algorithm
        // TODO: Add support for this algorithm
        game.log << "VQA: Decoding SND chunk - sound compressed using unsupported westwood algorithm" << endl;
        //Sound::WSADPCM_Decode(outbuf, inbuf, chunklen, uncompressed_size)
        chunklen = 0;
        break;
    case '2': // IMA ADPCM algorithm 
        Sound::IMADecode(outbuf, inbuf, chunklen, sndsample, sndindex);
        chunklen <<= 2; // IMA ADPCM decompresses sound to a size 4 times larger than the compressed size 
        break;
    default:
        game.log << "VQA: Decoding SND chunk - sound in unknown format" << endl;
        chunklen = 0;
        break;
    }

    delete[] inbuf;

    return chunklen;
}

/** Decodes VQFR Chunk into one frame(?)
 * @param pointer to decoded frame
 * @param Current Frame to decode
 */
bool VQAMovie::DecodeVQFRChunk(SDL_Surface *frame)
{
    unsigned int chunkid;
    unsigned int chunklen;
    unsigned char HiVal, LoVal;
    unsigned char CmpCBP, compressed; // Is CBP Look up table compressed or not
    int cpixel, bx, by, fpixel;
    SDL_Color CPL[256];

    if (vqafile->tell() & 1) {
        vqafile->seekCur(1);
    }

    vqafile->readDWord(&chunkid, 1);
    if (chunkid != vqa_vqfr_id) {
        game.log << "VQA: Decoding VQFR chunk - Expected 0x"
                 << std::hex << vqa_vqfr_id << ", got 0x" << chunkid
                 << std::dec << endl;
        return false;
    }

    vqafile->readDWord(&chunklen, 1);
    chunklen = SDL_Swap32(chunklen);
    CmpCBP = 0;

    bool done = false;
    // Read chunks until we get to the VPT chunk
    while (!done) {
        if (vqafile->tell() & 1)
            vqafile->seekCur(1);

        vqafile->readDWord(&chunkid, 1);
        compressed = VQA_HI_BYTE(chunkid) == 'Z';
        chunkid = chunkid & vqa_t_mask;
        switch (chunkid) {
        case vqa_cpl_id:
            DecodeCPLChunk(CPL);
            SDL_SetColors(frame, CPL, 0, header.Colors);
            break;
        case vqa_cbf_id:
            DecodeCBFChunk(compressed);
            break;
        case vqa_cbp_id:
            CmpCBP = compressed;
            DecodeCBPChunk();
            break;
        case vqa_vpt_id:
            // This chunk is always the last one 
            DecodeVPTChunk(compressed);
            done = true;
            break;
        default:
            DecodeUnknownChunk();
        }
    }

    // Optimise me harder
    cpixel = 0;
    fpixel = 0;
    for (by = 0; by < header.Height; by += header.BlockH) {
        for (bx = 0; bx < header.Width; bx += header.BlockW) {
            LoVal = VPT_Table[fpixel]; // formerly known as TopVal 
            HiVal = VPT_Table[fpixel + lowoffset]; // formerly known as LowVal 
            if (HiVal == modifier) {
                memset((unsigned char *)frame->pixels + cpixel, LoVal, header.BlockW);
                memset((unsigned char *)frame->pixels + cpixel + header.Width, LoVal, header.BlockW);
            } else {
                memcpy((unsigned char *)frame->pixels + cpixel, &CBF_LookUp[((HiVal<<8)|LoVal)<<3], header.BlockW);
                memcpy((unsigned char *)frame->pixels + cpixel + header.Width,
                       &CBF_LookUp[(((HiVal<<8)|LoVal)<<3)+4], header.BlockW);
            }
            cpixel += header.BlockW;
            fpixel++;
        }
        cpixel += header.Width;
    }

    //    if( !((CurFrame + 1) % header.CBParts) ) {
    if (CBPChunks & ~7) {
        if (CmpCBP == 1) {
            unsigned char CBPUNZ[0x0ff00 << 3];
            Compression::decode80(CBP_LookUp, CBPUNZ);
            memcpy(CBF_LookUp, CBPUNZ, 0x0ff00 << 3);
        } else {
            memcpy(CBF_LookUp, CBP_LookUp, 0x0ff00 << 3);
        }
        CBPOffset = 0;
        CBPChunks = 0;
    }
    return true;
}

inline void VQAMovie::DecodeCBPChunk()
{
    unsigned int chunklen;

    vqafile->readDWord(&chunklen, 1);
    chunklen = SDL_Swap32(chunklen);

    vqafile->readByte(CBP_LookUp + CBPOffset, chunklen);
    CBPOffset += chunklen;
    CBPChunks++;
}

inline void VQAMovie::DecodeVPTChunk(unsigned char Compressed)
{
    unsigned int chunklen;

    vqafile->readDWord( &chunklen, 1);
    chunklen = SDL_Swap32(chunklen);

    if (Compressed) {
        unsigned char *VPTZ; // Compressed VPT_Table 
        VPTZ = new unsigned char[chunklen];
        vqafile->readByte(VPTZ, chunklen);
        Compression::decode80(VPTZ, VPT_Table);
        delete[] VPTZ;
    } else { // uncompressed VPT chunk. never found any.. but might be some 
        vqafile->readByte(VPT_Table, chunklen);
    }
}

inline void VQAMovie::DecodeCBFChunk(unsigned char Compressed)
{
    unsigned int chunklen;

    vqafile->readDWord(&chunklen, 1);
    chunklen = SDL_Swap32(chunklen);

    if (Compressed) {
        unsigned char *CBFZ; // Compressed CBF table 
        CBFZ = new unsigned char[chunklen];
        vqafile->readByte(CBFZ, chunklen);
        Compression::decode80(CBFZ, CBF_LookUp);
        delete[] CBFZ;
    } else {
        vqafile->readByte(CBF_LookUp, chunklen);
    }
}

inline void VQAMovie::DecodeCPLChunk(SDL_Color *palette)
{
    unsigned int chunklen;
    int i;

    vqafile->readDWord(&chunklen, 1);
    chunklen = SDL_Swap32(chunklen);

    for (i = 0; i < header.Colors; i++) {
        vqafile->readByte((unsigned char*)&palette[i], 3);
        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
    }
}

inline void VQAMovie::DecodeUnknownChunk()
{
    unsigned int chunklen;

    vqafile->readDWord(&chunklen, 1);
    chunklen = SDL_Swap32(chunklen);
    game.log << "Unknown chunk at " << std::hex << vqafile->tell() << " for "
             << std::dec << chunklen << " bytes." << endl;

    vqafile->seekCur(chunklen);
}
