// taken from https://raw.githubusercontent.com/SaturnSH2x2/Sonic-CD-11-3DS/master/RSDKv3/Audio.hpp

#ifndef AUDIO_H
#define AUDIO_H

#include <stdlib.h>

#if RETRO_PLATFORM != RETRO_3DS
#include <vorbis/vorbisfile.h>
#endif

#if RETRO_PLATFORM != RETRO_VITA
#if RETRO_PLATFORM != RETRO_3DS
#include "SDL.h"
#endif
#endif

#define TRACK_COUNT (0x10)
#define SFX_COUNT (0x100)
#define CHANNEL_COUNT (0x4)
#define SFXDATA_COUNT (0x400000)

#define MAX_VOLUME (100)

#if RETRO_USING_SDLMIXER
#define AUDIO_FREQUENCY (44100)
#define AUDIO_FORMAT    (AUDIO_S16SYS) /**< Signed 16-bit samples */
#define AUDIO_SAMPLES   (0x800)
#define AUDIO_CHANNELS  (2)
#endif

struct TrackInfo {
    char fileName[0x40];
    bool trackLoop;
    uint loopPoint;

#if RETRO_USING_SDLMIXER
    Mix_Music* mus;
#endif
};

struct MusicPlaybackInfo {
    OggVorbis_File vorbisFile;
    int vorbBitstream;
#if RETRO_USING_SDL1_AUDIO
    SDL_AudioSpec spec;
#endif
#if RETRO_USING_SDL2
    SDL_AudioStream *stream;
// TODO: this is only here to make the compiler shut up,
// come up with a proper implementation later
#elif RETRO_PLATFORM == RETRO_3DS
    char* musicFile;
    void* stream;
    void* currentTrack;
    int pos;
    int len;
#endif
    Sint16 *buffer;
    FileInfo fileInfo;
    bool trackLoop;
    uint loopPoint;
    bool loaded;
};

struct SFXInfo {
    char name[0x40];
    Sint16 *buffer;
    size_t length;
    bool loaded;

#if RETRO_USING_SDLMIXER
    Mix_Chunk* chunk;
#endif
};

struct ChannelInfo {
    size_t sampleLength;
    Sint16 *samplePtr;
    int sfxID;
    byte loopSFX;
    sbyte pan;
};

enum MusicStatuses {
    MUSIC_STOPPED = 0,
    MUSIC_PLAYING = 1,
    MUSIC_PAUSED  = 2,
    MUSIC_LOADING = 3,
    MUSIC_READY   = 4,
};

extern int globalSFXCount;
extern int stageSFXCount;

extern int masterVolume;
extern int trackID;
extern int sfxVolume;
extern int bgmVolume;
extern bool audioEnabled;
extern bool globalSfxLoaded;

extern int nextChannelPos;
extern bool musicEnabled;
extern int musicStatus;
extern TrackInfo musicTracks[TRACK_COUNT];
extern SFXInfo sfxList[SFX_COUNT];

extern ChannelInfo sfxChannels[CHANNEL_COUNT];

extern MusicPlaybackInfo musInfo;

#if RETRO_USING_SDLMIXER
extern byte* trackData[TRACK_COUNT];
extern SDL_RWops* trackRwops[TRACK_COUNT];
extern byte* sfxData[SFX_COUNT];
extern SDL_RWops* sfxRwops[SFX_COUNT];
#endif

#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2
extern SDL_AudioSpec audioDeviceFormat;
#endif

int InitAudioPlayback();
void LoadGlobalSfx();

#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2
void ProcessMusicStream(void *data, Sint16 *stream, int len);
void ProcessAudioPlayback(void *data, Uint8 *stream, int len);
void ProcessAudioMixing(Sint32 *dst, const Sint16 *src, int len, int volume, sbyte pan);


inline void freeMusInfo()
{
    if (musInfo.loaded) {
        SDL_LockAudio();

        if (musInfo.buffer)
            delete[] musInfo.buffer;

#if RETRO_USING_SDL2
        if (musInfo.stream)
            SDL_FreeAudioStream(musInfo.stream);
#endif
        ov_clear(&musInfo.vorbisFile);
        musInfo.buffer       = nullptr;
#if RETRO_USING_SDL2
        musInfo.stream = nullptr;
#endif
        musInfo.trackLoop    = false;
        musInfo.loopPoint    = 0;
        musInfo.loaded       = false;
        musicStatus          = MUSIC_STOPPED;

        SDL_UnlockAudio();
    }
}
#else
void ProcessMusicStream();
void ProcessAudioPlayback();
void ProcessAudioMixing(Sint32 *dst, const Sint16 *src, int len, int volume, sbyte pan);

#if RETRO_USING_SDLMIXER
inline void FreeAllMusic() {
    Mix_HaltMusic();
    for (int i = 0; i < TRACK_COUNT; i++)
	if (musicTracks[i].mus != NULL) {
            Mix_FreeMusic(musicTracks[i].mus);
	    musicTracks[i].mus = NULL;
	    //SDL_RWclose(trackRwops[i]);
	    trackRwops[i] = NULL;
	    free(trackData[i]);
	    trackData[i] = NULL;
	}
}

inline void FreeAllSfx() {
    for (int i = 0; i < SFX_COUNT; i++) {
	    if (sfxList[i].chunk) {
	        Mix_FreeChunk(sfxList[i].chunk);
                sfxList[i].chunk = NULL;
	    }
	    //SDL_RWclose(sfxRwops[i]);
	    sfxRwops[i] = NULL;
	    //free(sfxData[i]);
	    sfxData[i] = NULL;
    }
}
#endif


inline void freeMusInfo()
{
    if (musInfo.loaded) {
        SDL_LockAudio();

        if (musInfo.buffer)
            delete[] musInfo.buffer;
        ov_clear(&musInfo.vorbisFile);
        musInfo.buffer    = nullptr;
        musInfo.trackLoop = false;
        musInfo.loopPoint = 0;
        musInfo.loaded    = false;
        musicStatus       = MUSIC_STOPPED;

        SDL_UnlockAudio();
    }
}

#if RETRO_USE_MOD_LOADER
extern char globalSfxNames[SFX_COUNT][0x40];
extern char stageSfxNames[SFX_COUNT][0x40];
void SetSfxName(const char *sfxName, int sfxID, bool global);
#endif
#endif

void SetMusicTrack(char *filePath, byte trackID, bool loop, uint loopPoint);
bool PlayMusic(int track);
inline void StopMusic()
{
#if RETRO_USING_SDLMIXER
    Mix_HaltMusic();
    musicStatus = MUSIC_STOPPED;
    freeMusInfo();
#else
    SDL_LockAudio();
    musicStatus = MUSIC_STOPPED;
    SDL_UnlockAudio();
    freeMusInfo();
#endif
}
void LoadSfx(char *filePath, byte sfxID);
void PlaySfx(int sfx, bool loop);
inline void StopSfx(int sfx)
{
#if RETRO_USING_SDLMIXER
    Mix_HaltChannel(-1);
#else
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (sfxChannels[i].sfxID == sfx) {
            MEM_ZERO(sfxChannels[i]);
            sfxChannels[i].sfxID = -1;
        }
    }
#endif
}
void SetSfxAttributes(int sfx, int loopCount, sbyte pan);

inline void SetMusicVolume(int volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > MAX_VOLUME)
        volume = MAX_VOLUME;
    masterVolume = volume;

#if RETRO_USING_SDLMIXER
    Mix_VolumeMusic((int) volume * 1.28);
#endif
}

inline bool PauseSound()
{
#if RETRO_USING_SDLMIXER
	Mix_PauseMusic();
#endif

    if (musicStatus == MUSIC_PLAYING) {
        musicStatus = MUSIC_PAUSED;
        return true;
    }
    return false;
}

inline void ResumeSound()
{
#if RETRO_USING_SDLMIXER
    Mix_ResumeMusic();
#endif

    if (musicStatus == MUSIC_PAUSED)
        musicStatus = MUSIC_PLAYING;
}


inline void StopAllSfx()
{
    for (int i = 0; i < CHANNEL_COUNT; ++i) sfxChannels[i].sfxID = -1;
}
inline void ReleaseGlobalSfx()
{
    StopAllSfx();
    for (int i = globalSFXCount - 1; i >= 0; --i) {
        if (sfxList[i].loaded) {
#if RETRO_USING_SDLMIXER
	    if (sfxList[i].chunk) {
                Mix_FreeChunk(sfxList[i].chunk);
	        sfxList[i].chunk = NULL;
	    }
	    //SDL_RWclose(sfxRwops[i]);
	    sfxRwops[i] = NULL;
	    //free(sfxData[i]);
	    sfxData[i] = NULL;
#endif

            StrCopy(sfxList[i].name, "");
            free(sfxList[i].buffer);
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }
    globalSFXCount = 0;
    globalSfxLoaded = false;
}
inline void ReleaseStageSfx()
{
    for (int i = stageSFXCount + globalSFXCount; i >= globalSFXCount; --i) {
        if (sfxList[i].loaded) {
#if RETRO_USING_SDLMIXER
	    if (sfxList[i].chunk) {
	        Mix_FreeChunk(sfxList[i].chunk);
                sfxList[i].chunk = NULL;
	    }
	    //SDL_RWclose(sfxRwops[i]);
	    sfxRwops[i] = NULL;
	    //free(sfxData[i]);
	    sfxData[i] = NULL;
#endif

            StrCopy(sfxList[i].name, "");
            free(sfxList[i].buffer);
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }
    stageSFXCount = 0;
}

inline void ReleaseAudioDevice()
{
    StopMusic();
    StopAllSfx();
    ReleaseStageSfx();
    ReleaseGlobalSfx();

#if RETRO_USING_SDLMIXER
    Mix_CloseAudio();
#endif
}

#endif // !AUDIO_H