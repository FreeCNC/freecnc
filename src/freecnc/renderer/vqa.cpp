#include <algorithm>
#include <functional>
#include <cassert>
#include <cctype>
#include <iomanip>
#include <iterator>
#include <stdexcept>

#include "../lib/compression.h"
#include "../sound/sound_public.h"
#include "graphicsengine.h"
#include "imageproc.h"
#include "vqa.h"
#include "../lib/fcncendian.h"

using std::distance;
using std::advance;

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

    unsigned int get_chunklen(shared_ptr<File>& file)
    {
        vector<unsigned char> data(4);
        file->read(data, 4);
        unsigned int dword = *reinterpret_cast<unsigned int*>(&data[0]);
        return big_endian(dword);
    }
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
VQAMovie::VQAMovie(const string& filename)
{
    if (filename.empty()) {
        throw VQAError("VQA: Empty filename");
    }
    vqafile = game.vfs.open(filename + ".VQA");

    if (!vqafile) {
        throw VQAError("VQA: Unable to open '" + filename + "'.VQA");
    }

    if (!DecodeFORMChunk()) {
        throw VQAError("VQA: (" + filename + ")Corrupt Header: Invalid FORM chunk");
    }

    // DecodeFINFChunk uses offsets
    offsets.resize(header.NumFrames);
    if (!DecodeFINFChunk()) {
        throw VQAError("VQA: (" + filename + ") Corrupt Header: Invalid FINF chunk");
    }

    CBF_LookUp.resize(0x0ff00 << 3);
    CBP_LookUp.resize(0x0ff00 << 3);
    VPT_Table.resize(lowoffset<<1);
    CBPOffset = 0;
    CBPChunks = 0;

    scaleVideo = game.config.scale_movies;
    videoScaleQuality = game.config.scaler_quality;

    if (SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, header.Channels, header.Freq, SOUND_FORMAT, SOUND_CHANNELS, SOUND_FREQUENCY) < 0) {
        throw VQAError("Could not build SDL_BuildAudioCVT filter");
    }

    empty = SDL_CreateSemaphore(0);
    full = SDL_CreateSemaphore(1);
    sndBufLock = SDL_CreateMutex();

    sndbuf.resize(2 * SOUND_MAX_UNCOMPRESSED_SIZE);
    sndbufMaxEnd = sndbuf.size();
    sndbufStart  = 0;
    sndbufEnd    = 0;
}

VQAMovie::~VQAMovie()
{
    SDL_DestroySemaphore(empty);
    SDL_DestroySemaphore(full);
    SDL_DestroyMutex(sndBufLock);
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
    while ((vqa->sndbufEnd - vqa->sndbufStart) < static_cast<size_t>(len)) {
        SDL_UnlockMutex(vqa->sndBufLock);
        SDL_SemWait(vqa->empty);
        SDL_LockMutex(vqa->sndBufLock);
    }

    // Copy our buffered data into the stream
    memcpy(stream, &vqa->sndbuf[vqa->sndbufStart], len);

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
        memmove(&vqa->sndbuf[0], &vqa->sndbuf[vqa->sndbufStart], len);

        // Reset the start & ends
        vqa->sndbufStart = 0;
        vqa->sndbufEnd   = len;
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
    vqafile->seek_start(offsets[0]);

    sndindex = 0;
    sndsample = 0;

    // create the frame to store the image in. 
    frame = SDL_CreateRGBSurface(SDL_SWSURFACE, header.Width, header.Height, 8, 0, 0, 0, 0);

    // Initialise the scaler 
    if (scaleVideo)
        scaler.initVideoScale(frame, videoScaleQuality);

    unsigned int jumplen;
    unsigned int sndlen = DecodeSNDChunk(&sndbuf[sndbufEnd]);
    sndbufEnd += sndlen;

    // The jump len is how far ahead we are allowed to be
    jumplen = sndlen;

    // Start music (aka the sound)
    pc::sfxeng->SetMusicHook(AudioHook, this);
    for (unsigned int framenum = 0; framenum < header.NumFrames; ++framenum) {
        SDL_LockMutex(sndBufLock);

        // Decode SND Chunk first 
        sndlen = DecodeSNDChunk(&sndbuf[sndbufEnd]);
        sndbufEnd += sndlen;

        // Signal we have more audio data
        SDL_SemPost(empty);

        // Wait until the buffer has been emptied again
        while ((sndbufEnd - sndbufStart) >= (size_t)(jumplen - (2 * sndlen))) {
            SDL_UnlockMutex(sndBufLock);
            SDL_SemWait(full);
            SDL_LockMutex(sndBufLock);
        }

        SDL_UnlockMutex(sndBufLock);

        if (!DecodeVQFRChunk(frame)) {
            game.log << "VQA: Decoding VQFR Chunk failed at frame " << framenum << endl;
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
    // Four bytes for FORM and four bytes for the chunk length
    int total_size = header_size + 8;
    vector<unsigned char> data(total_size);
    vqafile->read(data, total_size);

    if (data[0] != 'F' || data[1] != 'O' || data[2] != 'R' || data[3] != 'M') {
        game.log << "VQA: Malformed header in FORM chunk" << endl;
        return false;
    }

    vector<unsigned char>::iterator it = data.begin();
    // skip FORM and chunklen
    advance(it, 8);

    copy(it, it+8, header.Signature);
    advance(it, 8);
    header.RStartPos = read_dword(it, FCNC_BIG_ENDIAN);
    header.Version   = read_word(it, FCNC_LIL_ENDIAN);
    header.Flags     = read_word(it, FCNC_LIL_ENDIAN);
    header.NumFrames = read_word(it, FCNC_LIL_ENDIAN);
    header.Width     = read_word(it, FCNC_LIL_ENDIAN);
    header.Height    = read_word(it, FCNC_LIL_ENDIAN);
    header.BlockW    = read_byte(it);
    header.BlockH    = read_byte(it);
    header.FrameRate = read_byte(it);
    header.CBParts   = read_byte(it);
    header.Colors    = read_word(it, FCNC_LIL_ENDIAN);
    header.MaxBlocks = read_word(it, FCNC_LIL_ENDIAN);
    header.Unknown1  = read_word(it, FCNC_LIL_ENDIAN);
    header.Unknown2  = read_dword(it, FCNC_LIL_ENDIAN);
    header.Freq      = read_word(it, FCNC_LIL_ENDIAN);
    header.Channels  = read_byte(it);
    header.Bits      = read_byte(it);

    copy(it, it+sizeof(header.Unknown3), header.Unknown3);

    if (strncmp((const char*)header.Signature, "WVQAVQHD", 8)) {
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
    vector<unsigned char> data(4);

    vqafile->read(data, 4);

    if (data[0] != 'F' || data[1] != 'I' || data[2] != 'N' || data[3] != 'F') {
        game.log << "VQA: Malformed header in FINF chunk" << endl;
        return false;
    }

    unsigned int chunklen = get_chunklen(vqafile);

    if (static_cast<unsigned int>(header.NumFrames << 2) != chunklen) {
        game.log << "VQA: Invalid chunk length (" << (header.NumFrames << 2) 
                 << " != " << chunklen << endl;
        return false;
    }

    data.resize(header.NumFrames * 4);
    vqafile->read(data, header.NumFrames * 4);

    vector<unsigned char>::iterator data_it = data.begin();
    vector<unsigned int>::iterator off_it = offsets.begin();
    for (unsigned int framenum = 0; framenum < header.NumFrames; ++framenum, ++off_it) {
        unsigned int value = read_dword(data_it, FCNC_LIL_ENDIAN);
        *off_it = (value & 0x3FFFFFFF) << 1;
    }

    return true;
}

/** Decodes SND Chunk
 * @param pointer to store the decoded chunk
 * @return length of chunk
 */
unsigned int VQAMovie::DecodeSNDChunk(unsigned char *outbuf)
{
    // No sound
    if (!(header.Flags & 1)) {
        return 0;
    }

    // seek to correct offset
    if (vqafile->pos() & 1) {
        vqafile->seek_cur(1);
    }

    unsigned int chunkid;
    {
        vector<unsigned char> data(4);
        vqafile->read(data, 4);
        chunkid = readlong(data, 0);
    }

    if ((chunkid & vqa_t_mask) != vqa_snd_id) {
        game.log << "VQA: Decoding SND chunk - Expected 0x"
                 << std::hex << vqa_snd_id << ", got 0x" << (chunkid & vqa_t_mask)
                 << std::dec << endl;
        return 0; // Returning zero here, to set length of sound chunk to zero
    }

    int chunklen = get_chunklen(vqafile);

    SampleBuffer inbuf(chunklen);

    vqafile->read(inbuf, chunklen);

    switch (VQA_HI_BYTE(chunkid)) {
    case '0': // Raw uncompressed wave data 
        memcpy(outbuf, &inbuf[0], chunklen);
        break;
    case '1': // Westwoods own algorithm
        // TODO: Add support for this algorithm
        game.log << "VQA: Decoding SND chunk - sound compressed using unsupported westwood algorithm" << endl;
        //Sound::WSADPCM_Decode(outbuf, inbuf.begin(), chunklen, uncompressed_size)
        chunklen = 0;
        break;
    case '2': // IMA ADPCM algorithm 
        Sound::IMADecode(outbuf, inbuf.begin(), chunklen, sndsample, sndindex);
        chunklen <<= 2; // IMA ADPCM decompresses sound to a size 4 times larger than the compressed size 
        break;
    default:
        game.log << "VQA: Decoding SND chunk - sound in unknown format" << endl;
        chunklen = 0;
        break;
    }

    return chunklen;
}

/** Decodes VQFR Chunk into one frame(?)
 * @param pointer to decoded frame
 * @param Current Frame to decode
 */
bool VQAMovie::DecodeVQFRChunk(SDL_Surface *frame)
{
    unsigned char HiVal, LoVal;
    unsigned char CmpCBP, compressed; // Is CBP Look up table compressed or not
    int cpixel, bx, by, fpixel;
    SDL_Color CPL[256];

    if (vqafile->pos() & 1) {
        vqafile->seek_cur(1);
    }

    unsigned int chunkid;
    vector<unsigned char> id_data(8);
    vqafile->read(id_data, 8);
    chunkid = readlong(id_data, 0);
    // ignore chunklen in last four bytes

    if (chunkid != vqa_vqfr_id) {
        game.log << "VQA: Decoding VQFR chunk - Expected 0x"
                 << std::hex << vqa_vqfr_id << ", got 0x" << chunkid
                 << std::dec << endl;
        return false;
    }

    CmpCBP = 0;

    bool done = false;
    // Read chunks until we get to the VPT chunk
    while (!done) {
        if (vqafile->pos() & 1) {
            vqafile->seek_cur(1);
        }

        vqafile->read(id_data, 4);
        chunkid = readlong(id_data, 0);
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
            Compression::decode80(&CBP_LookUp[0], CBPUNZ);
            memcpy(&CBF_LookUp[0], CBPUNZ, 0x0ff00 << 3);
        } else {
            memcpy(&CBF_LookUp[0], &CBP_LookUp[0], 0x0ff00 << 3);
        }
        CBPOffset = 0;
        CBPChunks = 0;
    }
    return true;
}

inline void VQAMovie::DecodeCBPChunk()
{
    int chunklen = get_chunklen(vqafile);

    vector<unsigned char> chunk(chunklen);
    vqafile->read(chunk, chunklen);
    copy(chunk.begin(), chunk.end(), CBP_LookUp.begin() + CBPOffset);
    CBPOffset += chunklen;
    ++CBPChunks;
}

inline void VQAMovie::DecodeVPTChunk(unsigned char Compressed)
{
    int chunklen = get_chunklen(vqafile);

    if (Compressed) {
        vector<unsigned char> VPTZ(chunklen); // Compressed VPT_Table 
        vqafile->read(VPTZ, chunklen);
        Compression::decode80(&VPTZ[0], &VPT_Table[0]);
    } else { // uncompressed VPT chunk. never found any.. but might be some 
        vqafile->read(VPT_Table, chunklen);
    }
}

inline void VQAMovie::DecodeCBFChunk(unsigned char Compressed)
{
    int chunklen = get_chunklen(vqafile);

    if (Compressed) {
        vector<unsigned char> CBFZ(chunklen); // Compressed CBF table
        vqafile->read(CBFZ, chunklen);
        Compression::decode80(&CBFZ[0], &CBF_LookUp[0]);
    } else {
        vqafile->read(CBF_LookUp, chunklen);
    }
}

inline void VQAMovie::DecodeCPLChunk(SDL_Color *palette)
{
    int chunklen = get_chunklen(vqafile);

    int expected_size = header.Colors * 3;

    if (chunklen != expected_size) {
        game.log << "VQA: Malformed CPL header (got: " << chunklen
                 << ", expected " << expected_size << ")\n";
        throw VQAError("asdf");
    }

    vector<unsigned char> data(expected_size);
    vqafile->read(data, expected_size);
    for (int i = 0; i < header.Colors; i++) {
        palette[i].r = data[3 * i]     << 2;
        palette[i].g = data[3 * i + 1] << 2;
        palette[i].b = data[3 * i + 2] << 2;
    }
}

inline void VQAMovie::DecodeUnknownChunk()
{
    int chunklen = get_chunklen(vqafile);

    game.log << "VQA: Skipping unknown chunk at " << std::hex << vqafile->pos() << " for "
             << std::dec << chunklen << " bytes." << endl;

    vqafile->seek_cur(chunklen);
}
