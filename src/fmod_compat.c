/* FMOD5 Core API compatibility layer — exports FMOD5_* symbols
 * linking internally to DSA. LD_PRELOAD this as libfmod.so. */

#include "dsa.h"
#include <string.h>

/* FMOD uses different opaque pointer types but they're all just pointers.
 * DSA and FMOD have the same function signatures, so we can cast. */

#define EXPORT __attribute__((visibility("default")))

/* System */
EXPORT int FMOD5_System_Create(void **system, unsigned int version) {
    return DSA_System_Create((DSA_System **)system, version);
}
EXPORT int FMOD5_System_Release(void *system) {
    return DSA_System_Release((DSA_System *)system);
}
EXPORT int FMOD5_System_Init(void *system, int maxchannels, unsigned int flags, void *extradriverdata) {
    return DSA_System_Init((DSA_System *)system, maxchannels, flags, extradriverdata);
}
EXPORT int FMOD5_System_Close(void *system) {
    return DSA_System_Close((DSA_System *)system);
}
EXPORT int FMOD5_System_Update(void *system) {
    return DSA_System_Update((DSA_System *)system);
}
EXPORT int FMOD5_System_SetDSPBufferSize(void *system, unsigned int bufferlength, int numbuffers) {
    return DSA_System_SetDSPBufferSize((DSA_System *)system, bufferlength, numbuffers);
}
EXPORT int FMOD5_System_GetSoftwareFormat(void *system, int *samplerate, int *speakermode, int *numrawspeakers) {
    return DSA_System_GetSoftwareFormat((DSA_System *)system, samplerate, speakermode, numrawspeakers);
}
EXPORT int FMOD5_System_SetSoftwareFormat(void *system, int samplerate, int speakermode, int numrawspeakers) {
    return DSA_System_SetSoftwareFormat((DSA_System *)system, samplerate, speakermode, numrawspeakers);
}
EXPORT int FMOD5_System_Set3DSettings(void *system, float dopplerscale, float distancefactor, float rolloffscale) {
    return DSA_System_Set3DSettings((DSA_System *)system, dopplerscale, distancefactor, rolloffscale);
}
EXPORT int FMOD5_System_Get3DListenerAttributes(void *system, int listener, void *attrs) {
    return DSA_System_Get3DListenerAttributes((DSA_System *)system, listener, (DSA_3DAttributes *)attrs);
}
EXPORT int FMOD5_System_Set3DListenerAttributes(void *system, int listener, const void *attrs) {
    return DSA_System_Set3DListenerAttributes((DSA_System *)system, listener, (const DSA_3DAttributes *)attrs);
}
EXPORT int FMOD5_System_SetMemorySystem(void *system, const void *hooks) {
    return DSA_System_SetMemorySystem((DSA_System *)system, (const DSA_MemoryHooks *)hooks);
}
EXPORT int FMOD5_System_GetAdvancedSettings(void *system, void *settings) {
    return DSA_System_GetAdvancedSettings((DSA_System *)system, (DSA_AdvancedSettings *)settings);
}
EXPORT int FMOD5_System_SetAdvancedSettings(void *system, const void *settings) {
    return DSA_System_SetAdvancedSettings((DSA_System *)system, (const DSA_AdvancedSettings *)settings);
}
EXPORT int FMOD5_System_GetVersion(void *system, unsigned int *version) {
    return DSA_System_GetVersion((DSA_System *)system, version);
}
EXPORT int FMOD5_System_GetNumDrivers(void *system, int *numdrivers, int *which) {
    return DSA_System_GetNumDrivers((DSA_System *)system, numdrivers, which);
}
EXPORT int FMOD5_System_GetDriverInfo(void *system, int id, char *name, int namelen, void *guid, int *systemrate, int *speakermode, int *speakermodechannels) {
    return DSA_System_GetDriverInfo((DSA_System *)system, id, name, namelen, guid, systemrate, speakermode, speakermodechannels);
}

/* Sound */
EXPORT int FMOD5_System_CreateSound(void *system, const char *name, int mode, const void *exinfo, void **sound) {
    return DSA_System_CreateSound((DSA_System *)system, name, mode, (const DSA_CreateSoundExInfo *)exinfo, (DSA_Sound **)sound);
}
EXPORT int FMOD5_Sound_Release(void *sound) {
    return DSA_Sound_Release((DSA_Sound *)sound);
}
EXPORT int FMOD5_Sound_GetMode(void *sound, int *mode) {
    return DSA_Sound_GetMode((DSA_Sound *)sound, mode);
}
EXPORT int FMOD5_Sound_SetMode(void *sound, int mode) {
    return DSA_Sound_SetMode((DSA_Sound *)sound, mode);
}
EXPORT int FMOD5_Sound_GetLength(void *sound, unsigned int *length, int lengthunit) {
    return DSA_Sound_GetLength((DSA_Sound *)sound, length, lengthunit);
}
EXPORT int FMOD5_Sound_GetInfo(void *sound, void *info) {
    return DSA_Sound_GetInfo((DSA_Sound *)sound, (DSA_SoundInfo *)info);
}

/* Channel */
EXPORT int FMOD5_System_PlaySound(void *system, void *sound, void *group, int paused, void **channel) {
    return DSA_System_PlaySound((DSA_System *)system, (DSA_Sound *)sound, (DSA_ChannelGroup *)group, paused, (DSA_Channel **)channel);
}
EXPORT int FMOD5_Channel_SetVolume(void *channel, float volume) {
    return DSA_Channel_SetVolume((DSA_Channel *)channel, volume);
}
EXPORT int FMOD5_Channel_SetPitch(void *channel, float pitch) {
    return DSA_Channel_SetPitch((DSA_Channel *)channel, pitch);
}
EXPORT int FMOD5_Channel_SetPan(void *channel, float pan) {
    return DSA_Channel_SetPan((DSA_Channel *)channel, pan);
}
EXPORT int FMOD5_Channel_IsPlaying(void *channel, int *playing) {
    return DSA_Channel_IsPlaying((DSA_Channel *)channel, playing);
}
EXPORT int FMOD5_Channel_Stop(void *channel) {
    return DSA_Channel_Stop((DSA_Channel *)channel);
}
EXPORT int FMOD5_Channel_Set3DAttributes(void *channel, const void *attrs) {
    return DSA_Channel_Set3DAttributes((DSA_Channel *)channel, (const DSA_3DAttributes *)attrs);
}
EXPORT int FMOD5_Channel_Set3DMinMaxDistance(void *channel, float mindistance, float maxdistance) {
    return DSA_Channel_Set3DMinMaxDistance((DSA_Channel *)channel, mindistance, maxdistance);
}
EXPORT int FMOD5_Channel_GetFrequency(void *channel, float *frequency) {
    return DSA_Channel_GetFrequency((DSA_Channel *)channel, frequency);
}
EXPORT int FMOD5_Channel_SetFrequency(void *channel, float frequency) {
    return DSA_Channel_SetFrequency((DSA_Channel *)channel, frequency);
}
EXPORT int FMOD5_Channel_GetPosition(void *channel, unsigned int *position, int timeunit) {
    return DSA_Channel_GetPosition((DSA_Channel *)channel, position, timeunit);
}
EXPORT int FMOD5_Channel_SetPosition(void *channel, unsigned int position, int timeunit) {
    return DSA_Channel_SetPosition((DSA_Channel *)channel, position, timeunit);
}
EXPORT int FMOD5_Channel_GetCurrentSound(void *channel, void **sound) {
    return DSA_Channel_GetCurrentSound((DSA_Channel *)channel, (DSA_Sound **)sound);
}
EXPORT int FMOD5_Channel_GetChannelGroup(void *channel, void **group) {
    return DSA_Channel_GetChannelGroup((DSA_Channel *)channel, (DSA_ChannelGroup **)group);
}
EXPORT int FMOD5_Channel_AddDSP(void *channel, void *dsp, void **connection) {
    return DSA_Channel_AddDSP((DSA_Channel *)channel, (DSA_DSP *)dsp, (DSA_DSPConnection **)connection);
}

/* ChannelGroup */
EXPORT int FMOD5_System_GetMasterChannelGroup(void *system, void **group) {
    return DSA_System_GetMasterChannelGroup((DSA_System *)system, (DSA_ChannelGroup **)group);
}
EXPORT int FMOD5_ChannelGroup_Release(void *group) {
    return DSA_ChannelGroup_Release((DSA_ChannelGroup *)group);
}
EXPORT int FMOD5_ChannelGroup_Stop(void *group) {
    return DSA_ChannelGroup_Stop((DSA_ChannelGroup *)group);
}
EXPORT int FMOD5_ChannelGroup_AddDSP(void *group, void *dsp, void **connection) {
    return DSA_ChannelGroup_AddDSP((DSA_ChannelGroup *)group, (DSA_DSP *)dsp, (DSA_DSPConnection **)connection);
}
EXPORT int FMOD5_ChannelGroup_GetNumChannels(void *group, int *num) {
    return DSA_ChannelGroup_GetNumChannels((DSA_ChannelGroup *)group, num);
}
EXPORT int FMOD5_ChannelGroup_GetChannel(void *group, int index, void **channel) {
    return DSA_ChannelGroup_GetChannel((DSA_ChannelGroup *)group, index, (DSA_Channel **)channel);
}
EXPORT int FMOD5_ChannelGroup_SetVolume(void *group, float volume) {
    return DSA_ChannelGroup_SetVolume((DSA_ChannelGroup *)group, volume);
}
EXPORT int FMOD5_ChannelGroup_SetPitch(void *group, float pitch) {
    return DSA_ChannelGroup_SetPitch((DSA_ChannelGroup *)group, pitch);
}
EXPORT int FMOD5_ChannelGroup_GetVolume(void *group, float *volume) {
    return DSA_ChannelGroup_GetVolume((DSA_ChannelGroup *)group, volume);
}

/* DSP */
EXPORT int FMOD5_System_CreateDSPByType(void *system, int type, void **dsp) {
    return DSA_System_CreateDSPByType((DSA_System *)system, type, (DSA_DSP **)dsp);
}
EXPORT int FMOD5_DSP_Release(void *dsp) {
    return DSA_DSP_Release((DSA_DSP *)dsp);
}
EXPORT int FMOD5_DSP_GetInfo(void *dsp, char *name, unsigned int *version, int *channels, int *configwidth, int *configheight) {
    return DSA_DSP_GetInfo((DSA_DSP *)dsp, name, version, channels, configwidth, configheight);
}
EXPORT int FMOD5_DSP_AddDSP(void *dsp, void *connection, void **conn) {
    return DSA_DSP_AddDSP((DSA_DSP *)dsp, (DSA_DSP *)connection, (DSA_DSPConnection **)conn);
}
EXPORT int FMOD5_DSP_Remove(void *dsp) {
    return DSA_DSP_Remove((DSA_DSP *)dsp);
}
EXPORT int FMOD5_DSP_GetNumInputs(void *dsp, int *num) {
    return DSA_DSP_GetNumInputs((DSA_DSP *)dsp, num);
}
EXPORT int FMOD5_DSP_GetNumOutputs(void *dsp, int *num) {
    return DSA_DSP_GetNumOutputs((DSA_DSP *)dsp, num);
}
EXPORT int FMOD5_DSP_SetParameterFloat(void *dsp, int index, float value) {
    return DSA_DSP_SetParameterFloat((DSA_DSP *)dsp, index, value);
}
EXPORT int FMOD5_DSP_GetParameterFloat(void *dsp, int index, float *value) {
    return DSA_DSP_GetParameterFloat((DSA_DSP *)dsp, index, value);
}
EXPORT int FMOD5_DSP_SetActive(void *dsp, int active) {
    return DSA_DSP_SetActive((DSA_DSP *)dsp, active);
}

/* DSPConnection */
EXPORT int FMOD5_DSPConnection_SetMix(void *conn, float mix) {
    return DSA_DSPConnection_SetMix((DSA_DSPConnection *)conn, mix);
}
EXPORT int FMOD5_DSPConnection_GetMix(void *conn, float *mix) {
    return DSA_DSPConnection_GetMix((DSA_DSPConnection *)conn, mix);
}

/* SoundGroup */
EXPORT int FMOD5_System_CreateSoundGroup(void *system, const char *name, void **group) {
    return DSA_System_CreateSoundGroup((DSA_System *)system, name, (DSA_SoundGroup **)group);
}
EXPORT int FMOD5_SoundGroup_Release(void *group) {
    return DSA_SoundGroup_Release((DSA_SoundGroup *)group);
}
EXPORT int FMOD5_SoundGroup_GetNumPlaying(void *group, int *num) {
    return DSA_SoundGroup_GetNumPlaying((DSA_SoundGroup *)group, num);
}
EXPORT int FMOD5_SoundGroup_SetVolume(void *group, float volume) {
    return DSA_SoundGroup_SetVolume((DSA_SoundGroup *)group, volume);
}
EXPORT int FMOD5_SoundGroup_GetVolume(void *group, float *volume) {
    return DSA_SoundGroup_GetVolume((DSA_SoundGroup *)group, volume);
}

/* Geometry */
EXPORT int FMOD5_System_CreateGeometry(void *system, int maxpolygons, int maxvertices, void **geometry) {
    return DSA_System_CreateGeometry((DSA_System *)system, maxpolygons, maxvertices, (DSA_Geometry **)geometry);
}
EXPORT int FMOD5_Geometry_Release(void *geometry) {
    return DSA_Geometry_Release((DSA_Geometry *)geometry);
}
