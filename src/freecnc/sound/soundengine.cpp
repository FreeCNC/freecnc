#include <cassert>
#include <algorithm>
#include <functional>

#include "SDL_mixer.h"

#include "../freecnc.h"
#include "soundengine.h"
#include "soundfile.h"

using Sound::SoundCache;
using Sound::SoundBuffer;

//-----------------------------------------------------------------------------
// Functors
//-----------------------------------------------------------------------------

namespace
{
    struct SoundCacheCleaner : public std::unary_function<SoundCache::value_type, void>
    {
        void operator()(const SoundCache::value_type& p)
        {
            Mix_FreeChunk(p.second->chunk);
            delete p.second;
        }
    };

    SoundFile soundDecoder;
    SoundFile musicDecoder;
}

//-----------------------------------------------------------------------------
// Constructor / Destructor
//-----------------------------------------------------------------------------

SoundEngine::SoundEngine(bool disableSound) : nosound(disableSound), musicFinished(true), currentTrack(playlist.begin())
{
    /// @TODO Setting the chunksize parameter to 1024 eliminates choppiness in
    // music playback on my laptop.  Try this value on other systems.
    if (Mix_OpenAudio(SOUND_FREQUENCY, SOUND_FORMAT, SOUND_CHANNELS, 4096) < 0) {
        logger->error("Unable to open sound: %s\n", Mix_GetError());
        nosound = true;
    } else {
        // Set volumes to half of max by default; this should be a fallback, real setting should be read from config
        SetSoundVolume(MIX_MAX_VOLUME / 2);
        SetMusicVolume(MIX_MAX_VOLUME / 2);
    }
}

SoundEngine::~SoundEngine()
{
    if (nosound)
        return;

    // Stop all playback
    Mix_HaltChannel(-1);
    Mix_HookMusic(NULL, NULL);
    Mix_CloseAudio();
}

//-----------------------------------------------------------------------------
// Game Sounds
//-----------------------------------------------------------------------------

void SoundEngine::SetSoundVolume(unsigned int volume)
{
    if (nosound)
        return;

    assert(volume <= MIX_MAX_VOLUME);
    Mix_Volume(-1, volume);
    soundVolume = volume;
}

void SoundEngine::PlaySound(const std::string& sound)
{
    PlayLoopedSound(sound, 1);
}

int SoundEngine::PlayLoopedSound(const std::string& sound, unsigned int loops)
{
    if (nosound)
        return -1;

    SoundBuffer* snd = LoadSoundImpl(sound);
    if (!snd) {
        return -1;
    }

    int channel = Mix_PlayChannel(-1, snd->chunk, static_cast<int>(loops)-1);
    //Mix_Volume(channel, soundVolume);
    return channel;
}

void SoundEngine::StopLoopedSound(int id)
{
    if (nosound)
        return;

    Mix_HaltChannel(id);
}

//-----------------------------------------------------------------------------
// Music Playback
//-----------------------------------------------------------------------------

void SoundEngine::SetMusicVolume(unsigned int volume)
{
    if (nosound)
        return;

    assert(volume <= MIX_MAX_VOLUME);
    //Mix_VolumeMusic(volume);
}

bool SoundEngine::CreatePlaylist(const std::string& gamename)
{
    // Create the amazing playlist
    playlist.clear();
    playlist.push_back("aoi.aud");
    currentTrack = playlist.begin();

    return true;
}

void SoundEngine::PlayMusic()
{
    if (nosound)
        return;

    if (!Mix_PlayingMusic()) {
        if (Mix_PausedMusic()) {
            Mix_ResumeMusic();
        } else {
            PlayTrack(*currentTrack);
        }
    }
}

void SoundEngine::PauseMusic()
{
    if (nosound)
        return;

    Mix_PauseMusic();
}

void SoundEngine::StopMusic()
{
    if (nosound)
        return;

    Mix_HookMusic(NULL, NULL);
    musicDecoder.Close();
}

void SoundEngine::PlayTrack(const std::string& sound)
{
    if (nosound)
        return;

    StopMusic();

    if (sound == "No theme") {
        PlayMusic();
        return;
    }

    if (musicDecoder.Open(sound)) {
        musicFinished = false;
        Mix_HookMusic(MusicHook, reinterpret_cast<void*>(&musicFinished));
    }
}

void SoundEngine::NextTrack()
{
    if (nosound)
        return;

    if (++currentTrack == playlist.end())
        currentTrack = playlist.begin();

    PlayTrack(*currentTrack);
}

void SoundEngine::PrevTrack()
{
    if (nosound)
        return;

    if (currentTrack == playlist.begin())
        currentTrack = playlist.end();

    PlayTrack(*(--currentTrack));
}

void SoundEngine::MusicHook(void* userdata, Uint8* stream, int len)
{
    bool* musicFinished = reinterpret_cast<bool*>(userdata);
    if (!*musicFinished) {
        SampleBuffer buffer;
        Uint32 ret = musicDecoder.Decode(buffer, len);
        if (ret == SOUND_DECODE_COMPLETED) {
            musicDecoder.Close();
            *musicFinished = true;
        } else if (ret == SOUND_DECODE_ERROR) {
            logger->error("Sound: Error during music decoding, stopping playback of current track.\n");
            *musicFinished = true;
            return;
        }
        memcpy(stream, &buffer[0], buffer.size());
    }
}

void SoundEngine::SetMusicHook(MixFunc mixfunc, void* arg)
{
    Mix_HookMusic(mixfunc, arg);
}

//-----------------------------------------------------------------------------
// Sound loading, caching
//-----------------------------------------------------------------------------

void SoundEngine::LoadSound(const std::string& sound)
{
    LoadSoundImpl(sound);
}

SoundBuffer* SoundEngine::LoadSoundImpl(const std::string& sound)
{
    SoundBuffer* buffer = 0; 
  
    // Check if sound is already loaded and cached
    SoundCache::iterator cachedSound = soundCache.find(sound);
    if (cachedSound != soundCache.end()) {
       buffer = cachedSound->second;
    } else {
        // Load and cache sound
        if (soundDecoder.Open(sound)) {
            buffer = new SoundBuffer();       
            if (soundDecoder.Decode(buffer->data) == SOUND_DECODE_COMPLETED) {
                buffer->chunk = Mix_QuickLoad_RAW(&buffer->data[0], static_cast<Uint32>(buffer->data.size()));
                soundCache.insert(SoundCache::value_type(sound, buffer));
            } else {
                delete buffer;
                buffer = 0;
            }
            soundDecoder.Close();
        }
    }
    return buffer;
}
