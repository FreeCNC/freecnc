#ifndef _SOUND_SOUNDENGINE_H
#define _SOUND_SOUNDENGINE_H

#include "../freecnc.h"
#include "soundcommon.h"

#include "SDL_mixer.h"

namespace Sound
{
    struct SoundBuffer {
        SampleBuffer data;
        Mix_Chunk* chunk;
    };
    typedef map<string, SoundBuffer*> SoundCache; // Make this use shared_ptr
}

class SoundEngine
{
public:
    SoundEngine(bool disableSound = false);
    ~SoundEngine();

    void LoadSound(const std::string& sound);

    // Game sounds
    void SetSoundVolume(unsigned int volume);
    void PlaySound(const std::string& sound);
    int  PlayLoopedSound(const std::string& sound, unsigned int loops);
    void StopLoopedSound(int id);

    // Music
    void SetMusicVolume(unsigned int volume);
    bool CreatePlaylist(const std::string& gamename);
    void PlayMusic(); // Starts|Resumes playing
    void PauseMusic();
    void StopMusic();

    void PlayTrack(const std::string& sound); // Plays a specific track
    void NextTrack(); // Selects the next track in the playlist
    void PrevTrack(); // Selects the previous track in the playlist

    static void MusicHook(void* userdata, unsigned char* stream, int len);

    typedef void (*MixFunc)(void*, unsigned char*, int);
    void SetMusicHook(MixFunc mixfunc, void *arg);

    bool NoSound() const { return nosound; }

private:
    typedef std::vector<std::string> Playlist;

    Sound::SoundCache soundCache;

    bool nosound;
    bool musicFinished;
    unsigned int soundVolume;

    Playlist playlist;
    Playlist::iterator currentTrack;

    Sound::SoundBuffer *LoadSoundImpl(const std::string &sound);
};

#endif
