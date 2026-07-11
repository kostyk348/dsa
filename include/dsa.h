#ifndef DSA_H_
#define DSA_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* === Error codes (FMOD-compatible) === */
#define DSA_OK                       0
#define DSA_ERR_BADCOMMAND           1
#define DSA_ERR_CHANNEL_ALLOC        2
#define DSA_ERR_CHANNEL_STOLEN       3
#define DSA_ERR_FILE_NOTFOUND       18
#define DSA_ERR_FILE_BAD            13
#define DSA_ERR_FORMAT              19
#define DSA_ERR_INITIALIZATION      26
#define DSA_ERR_INITIALIZED         27
#define DSA_ERR_INTERNAL            28
#define DSA_ERR_INVALID_HANDLE      30
#define DSA_ERR_NOTREADY            31
#define DSA_ERR_OUTPUT_CREATEBUFFER 32
#define DSA_ERR_VERSION             33
#define DSA_ERR_TOOMANYCHANNELS     36
#define DSA_ERR_ALREADYLOCKED       38
#define DSA_ERR_NOTIMPLEMENTED      54

/* === Audio format detection === */
#define DSA_SOUNDFORMAT_RAW  0
#define DSA_SOUNDFORMAT_WAV  1
#define DSA_SOUNDFORMAT_MP3  2
#define DSA_SOUNDFORMAT_OGG  3

/* === Init flags === */
#define DSA_INIT_NORMAL              0x00000000
#define DSA_INIT_STREAM_FROM_UPDATE  0x00000001
#define DSA_INIT_3D_RIGHTHANDED      0x00000002
#define DSA_INIT_CHANNEL_LOWPASS     0x00000100
#define DSA_INIT_CHANNEL_DISTANCEFILTER 0x00000200
#define DSA_INIT_PROFILE_ENABLE      0x00010000

/* === Sound modes === */
#define DSA_DEFAULT                  0x00000000
#define DSA_LOOP_OFF                 0x00000001
#define DSA_LOOP_NORMAL              0x00000002
#define DSA_LOOP_BIDI                0x00000004
#define DSA_2D                       0x00000008
#define DSA_3D                       0x00000010
#define DSA_CREATESTREAM             0x00000080
#define DSA_CREATESAMPLE             0x00000100
#define DSA_OPENUSER                 0x00010000
#define DSA_OPENMEMORY               0x00020000
#define DSA_OPENMEMORY_POINT         0x04000000
#define DSA_SOFTWARE                 0x00002000

/* === DSP types === */
#define DSA_DSP_TYPE_UNKNOWN         0
#define DSA_DSP_TYPE_MIXER           1
#define DSA_DSP_TYPE_OSCILLATOR      2
#define DSA_DSP_TYPE_LOWPASS         3
#define DSA_DSP_TYPE_ITLOWPASS       4
#define DSA_DSP_TYPE_HIGHPASS        5
#define DSA_DSP_TYPE_ECHO            6
#define DSA_DSP_TYPE_FLANGE          7
#define DSA_DSP_TYPE_DISTORTION      8
#define DSA_DSP_TYPE_NORMALIZE       9
#define DSA_DSP_TYPE_PARAMEQ         10
#define DSA_DSP_TYPE_PITCHSHIFT      11
#define DSA_DSP_TYPE_REVERB          12
#define DSA_DSP_TYPE_SFXREVERB       13
#define DSA_DSP_TYPE_LOWPASS_SIMPLE  14
#define DSA_DSP_TYPE_DELAY           15
#define DSA_DSP_TYPE_TREMOLO         16
#define DSA_DSP_TYPE_COMPRESSOR      17

/* === Channel state === */
#define DSA_CHANNELSTATE_FREE        0
#define DSA_CHANNELSTATE_PLAYING     1
#define DSA_CHANNELSTATE_PAUSED      2
#define DSA_CHANNELSTATE_STOPPED     3

/* === DSP index constants === */
#define DSA_CHANNEL_FADER            -1
#define DSA_CHANNEL_MIXER            0
#define DSA_CHANNEL_PANNER           1

/* === Length units === */
#define DSA_TIMEUNIT_MS              0x00000002
#define DSA_TIMEUNIT_PCMBYTES        0x00000004
#define DSA_TIMEUNIT_PCMSAMPLES      0x00000008

/* === Caps / limits === */
#define DSA_MAX_CHANNELS             256
#define DSA_MAX_CHANNELGROUPS        64
#define DSA_MAX_SOUNDS               4096
#define DSA_MAX_DSP                  128
#define DSA_MAX_GEOMETRY             256
#define DSA_DEFAULT_SAMPLERATE       44100
#define DSA_DEFAULT_BUFFERLENGTH     1024
#define DSA_DEFAULT_NUMBUFFERS       4

/* Opaque handles */
typedef struct DSA_Sound          DSA_Sound;
typedef struct DSA_Channel        DSA_Channel;
typedef struct DSA_ChannelGroup   DSA_ChannelGroup;
typedef struct DSA_DSP            DSA_DSP;
typedef struct DSA_DSPConnection  DSA_DSPConnection;
typedef struct DSA_SoundGroup     DSA_SoundGroup;
typedef struct DSA_Geometry       DSA_Geometry;
typedef struct DSA_Reverb         DSA_Reverb;
typedef union  DSA_System         DSA_System;

/* === Memory hooks (optional — if NULL, use malloc/free) === */
typedef void *(*DSA_ALLOC_CALLBACK)(unsigned int size, unsigned int alignment, const char *type);
typedef void  (*DSA_FREE_CALLBACK)(void *ptr, unsigned int size, const char *type);

typedef struct {
    DSA_ALLOC_CALLBACK alloc;
    DSA_FREE_CALLBACK  free;
} DSA_MemoryHooks;

/* === Sound info struct === */
typedef struct {
    unsigned int length; /* in PCM samples */
    int channels;
    int bits_per_sample;
    int samplerate;
    int mode;
} DSA_SoundInfo;

/* === DSP info struct === */
typedef struct {
    char name[64];
    unsigned int version;
    int channels;
    int config_width;
    int config_height;
} DSA_DSPInfo;

/* === Advanced settings === */
typedef struct {
    int cb_size;
    unsigned int log_level;
    int user_3d_algorithm;
    unsigned int random_seed;
    int max_ambisonic_order;
    int profile_bank_memory;
} DSA_AdvancedSettings;

/* === CREATE SOUND EXINFO === */
typedef struct {
    int cb_size;
    unsigned int length;
    unsigned int fileoffset;
    int numchannels;
    int defaultfrequency;
    int format;
    int suggested_callduration;
    int file_size;
    void *userdata;
    int dls_version;
    void *encoded_info;
    void *pcmsamples;
} DSA_CreateSoundExInfo;

/* === 3D attributes === */
typedef struct {
    float position[3];
    float velocity[3];
    float forward[3];
    float up[3];
} DSA_3DAttributes;

/* === System API === */
int DSA_System_Create(DSA_System **system, unsigned int version);
int DSA_System_Release(DSA_System *system);
int DSA_System_Init(DSA_System *system, int max_channels, unsigned int flags, void *extradriverdata);
int DSA_System_Close(DSA_System *system);
int DSA_System_Update(DSA_System *system);
int DSA_System_SetDSPBufferSize(DSA_System *system, unsigned int bufferlength, int numbuffers);
int DSA_System_GetSoftwareFormat(DSA_System *system, int *samplerate, int *speakermode, int *numrawspeakers);
int DSA_System_SetSoftwareFormat(DSA_System *system, int samplerate, int speakermode, int numrawspeakers);
int DSA_System_Set3DSettings(DSA_System *system, float dopplerscale, float distancefactor, float rolloffscale);
int DSA_System_Get3DListenerAttributes(DSA_System *system, int listener, DSA_3DAttributes *attrs);
int DSA_System_Set3DListenerAttributes(DSA_System *system, int listener, const DSA_3DAttributes *attrs);
int DSA_System_SetMemorySystem(DSA_System *system, const DSA_MemoryHooks *hooks);
int DSA_System_GetAdvancedSettings(DSA_System *system, DSA_AdvancedSettings *settings);
int DSA_System_SetAdvancedSettings(DSA_System *system, const DSA_AdvancedSettings *settings);
int DSA_System_GetVersion(DSA_System *system, unsigned int *version);
int DSA_System_GetNumDrivers(DSA_System *system, int *numdrivers, int *which);
int DSA_System_GetDriverInfo(DSA_System *system, int id, char *name, int namelen, void *guid, int *systemrate, int *speakermode, int *speakermodechannels);

/* === Sound API === */
int DSA_System_CreateSound(DSA_System *system, const char *name, int mode, const DSA_CreateSoundExInfo *exinfo, DSA_Sound **sound);
/* Get last created Core sound (used by Studio EventInstance bridge) */
int DSA_System_GetLastSound(DSA_System *system, DSA_Sound **sound);
int DSA_Sound_Release(DSA_Sound *sound);
int DSA_Sound_GetMode(DSA_Sound *sound, int *mode);
int DSA_Sound_GetLength(DSA_Sound *sound, unsigned int *length, int lengthunit);
int DSA_Sound_GetInfo(DSA_Sound *sound, DSA_SoundInfo *info);
int DSA_Sound_SetMode(DSA_Sound *sound, int mode);

/* === Channel API === */
int DSA_System_PlaySound(DSA_System *system, DSA_Sound *sound, DSA_ChannelGroup *group, int paused, DSA_Channel **channel);
int DSA_Channel_SetVolume(DSA_Channel *channel, float volume);
int DSA_Channel_SetPitch(DSA_Channel *channel, float pitch);
int DSA_Channel_SetPan(DSA_Channel *channel, float pan);
int DSA_Channel_IsPlaying(DSA_Channel *channel, int *playing);
int DSA_Channel_Stop(DSA_Channel *channel);
int DSA_Channel_Set3DAttributes(DSA_Channel *channel, const DSA_3DAttributes *attrs);
int DSA_Channel_Set3DMinMaxDistance(DSA_Channel *channel, float mindistance, float maxdistance);
int DSA_Channel_GetFrequency(DSA_Channel *channel, float *frequency);
int DSA_Channel_SetFrequency(DSA_Channel *channel, float frequency);
int DSA_Channel_GetPosition(DSA_Channel *channel, unsigned int *position, int timeunit);
int DSA_Channel_SetPosition(DSA_Channel *channel, unsigned int position, int timeunit);
int DSA_Channel_GetCurrentSound(DSA_Channel *channel, DSA_Sound **sound);
int DSA_Channel_GetChannelGroup(DSA_Channel *channel, DSA_ChannelGroup **group);
int DSA_Channel_AddDSP(DSA_Channel *channel, DSA_DSP *dsp, DSA_DSPConnection **connection);

/* === ChannelGroup API === */
int DSA_System_GetMasterChannelGroup(DSA_System *system, DSA_ChannelGroup **group);
int DSA_ChannelGroup_Release(DSA_ChannelGroup *group);
int DSA_ChannelGroup_Stop(DSA_ChannelGroup *group);
int DSA_ChannelGroup_AddDSP(DSA_ChannelGroup *group, DSA_DSP *dsp, DSA_DSPConnection **connection);
int DSA_ChannelGroup_GetNumChannels(DSA_ChannelGroup *group, int *num);
int DSA_ChannelGroup_GetChannel(DSA_ChannelGroup *group, int index, DSA_Channel **channel);
int DSA_ChannelGroup_SetVolume(DSA_ChannelGroup *group, float volume);
int DSA_ChannelGroup_SetPitch(DSA_ChannelGroup *group, float pitch);
int DSA_ChannelGroup_GetVolume(DSA_ChannelGroup *group, float *volume);

/* === DSP API === */
int DSA_System_CreateDSPByType(DSA_System *system, int type, DSA_DSP **dsp);
int DSA_DSP_Release(DSA_DSP *dsp);
int DSA_DSP_GetInfo(DSA_DSP *dsp, char *name, unsigned int *version, int *channels, int *configwidth, int *configheight);
int DSA_DSP_AddDSP(DSA_DSP *dsp, DSA_DSP *connection, DSA_DSPConnection **conn);
int DSA_DSP_Remove(DSA_DSP *dsp);
int DSA_DSP_GetNumInputs(DSA_DSP *dsp, int *num);
int DSA_DSP_GetNumOutputs(DSA_DSP *dsp, int *num);
int DSA_DSP_SetParameterFloat(DSA_DSP *dsp, int index, float value);
int DSA_DSP_GetParameterFloat(DSA_DSP *dsp, int index, float *value);
int DSA_DSP_SetActive(DSA_DSP *dsp, int active);

/* === DSPConnection API === */
int DSA_DSPConnection_SetMix(DSA_DSPConnection *conn, float mix);
int DSA_DSPConnection_GetMix(DSA_DSPConnection *conn, float *mix);

/* === SoundGroup API === */
int DSA_System_CreateSoundGroup(DSA_System *system, const char *name, DSA_SoundGroup **group);
int DSA_SoundGroup_Release(DSA_SoundGroup *group);
int DSA_SoundGroup_GetNumPlaying(DSA_SoundGroup *group, int *num);
int DSA_SoundGroup_SetVolume(DSA_SoundGroup *group, float volume);
int DSA_SoundGroup_GetVolume(DSA_SoundGroup *group, float *volume);

/* === Geometry API === */
int DSA_System_CreateGeometry(DSA_System *system, int maxpolygons, int maxvertices, DSA_Geometry **geometry);
int DSA_Geometry_Release(DSA_Geometry *geometry);

#ifdef __cplusplus
}
#endif

#endif /* DSA_H_ */
