/* FMOD C++ ABI compatibility layer — exports _ZN4FMOD* mangled symbols.
 * Written in C (name is exact symbol, no mangling). 
 * Each "this" call is mapped to DSA C API.
 * LD_PRELOAD this as libfmod.so alongside fmod_compat.c. */

#include "dsa.h"
#include <string.h>
#include <stdio.h>

#define EXPORT __attribute__((visibility("default")))
#define FMOD_OK      0
#define FMOD_ERR_BADCOMMAND  1
#define FMOD_ERR_NOTIMPLEMENTED 54

/* =====================================================
 * C symbols needed by studio
 * ===================================================== */
EXPORT int FMOD_System_Create(void **system, unsigned int version) {
    return DSA_System_Create((DSA_System **)system, version);
}
EXPORT int FMOD_Memory_GetStats(int *currentalloced, int *maxalloced, int blocking) {
    if (currentalloced) *currentalloced = 0;
    if (maxalloced) *maxalloced = 0;
    (void)blocking;
    return FMOD_OK;
}
EXPORT int FMOD_Debug_Initialize(unsigned int flags, int mode, void *callback, const char *filename) {
    (void)flags; (void)mode; (void)callback; (void)filename;
    return FMOD_OK;
}

/* =====================================================
 * FMOD::Global
 * ===================================================== */
EXPORT int _ZN4FMOD10getGlobalsEPPNS_6GlobalE(void **globals) {
    if (globals) *globals = (void*)1; /* dummy non-null */
    return FMOD_OK;
}

/* =====================================================
 * FMOD::SystemI (internal)
 * ===================================================== */
EXPORT int _ZN4FMOD7SystemI14createDiskFileEPKcP22FMOD_CREATESOUNDEXINFOPPNS_4FileEPb(void *sys, const char *name, void *exinfo, void **file, int *fromdisk) {
    (void)sys; (void)name; (void)exinfo; (void)file;
    if (fromdisk) *fromdisk = 1;
    return FMOD_ERR_NOTIMPLEMENTED;
}
EXPORT int _ZN4FMOD7SystemI16createMemoryFileEPPNS_4FileE(void *sys, void **file) {
    (void)sys; (void)file;
    return FMOD_ERR_NOTIMPLEMENTED;
}
EXPORT int _ZN4FMOD7SystemI19createClientProfileEv(void *sys) {
    (void)sys;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD7SystemI19setInternalCallbackEiPF11FMOD_RESULTP11FMOD_SYSTEMjPvS4_S4_ES4_(void *sys, int cb_id, void *cb, void *userdata) {
    (void)sys; (void)cb_id; (void)cb; (void)userdata;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD7SystemI8validateEPNS_6SystemEPPS0_PNS_15SystemLockScopeE(void *sys, void *system, void **out, void *lock) {
    (void)sys; (void)lock;
    if (out) *out = system ? system : sys;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD8ChannelI8validateEPNS_7ChannelEPPS0_PNS_15SystemLockScopeE(void *sys, void *channel, void **out, void *lock) {
    (void)sys; (void)lock;
    if (out) *out = channel;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD4DSPI8validateEPNS_3DSPEPPS0_PNS_15SystemLockScopeE(void *sys, void *dsp, void **out, void *lock) {
    (void)sys; (void)lock;
    if (out) *out = dsp;
    return FMOD_OK;
}

/* =====================================================
 * FMOD::System
 * ===================================================== */
EXPORT int _ZN4FMOD6System4initEijPv(void *sys, int maxchannels, unsigned int flags, void *extradriverdata) {
    return DSA_System_Init((DSA_System *)sys, maxchannels, flags, extradriverdata);
}
EXPORT int _ZN4FMOD6System7releaseEv(void *sys) {
    return DSA_System_Release((DSA_System *)sys);
}
EXPORT int _ZN4FMOD6System6updateEv(void *sys) {
    return DSA_System_Update((DSA_System *)sys);
}
EXPORT int _ZN4FMOD6System9playSoundEPNS_5SoundEPNS_12ChannelGroupEbPPNS_7ChannelE(void *sys, void *sound, void *group, int paused, void **channel) {
    return DSA_System_PlaySound((DSA_System *)sys, (DSA_Sound *)sound, (DSA_ChannelGroup *)group, paused, (DSA_Channel **)channel);
}
EXPORT int _ZN4FMOD6System11createSoundEPKcjP22FMOD_CREATESOUNDEXINFOPPNS_5SoundE(void *sys, const char *name, unsigned int mode, void *exinfo, void **sound) {
    return DSA_System_CreateSound((DSA_System *)sys, name, (int)mode, (const DSA_CreateSoundExInfo *)exinfo, (DSA_Sound **)sound);
}
EXPORT int _ZN4FMOD6System11getCPUUsageEP14FMOD_CPU_USAGE(void *sys, void *usage) {
    (void)sys; (void)usage;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System11registerDSPEPK20FMOD_DSP_DESCRIPTIONPj(void *sys, const void *desc, unsigned int *handle) {
    (void)sys; (void)desc;
    if (handle) *handle = 1;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System12getFileUsageEPxS1_S1_(void *sys, long long *minor, long long *major, long long *max) {
    (void)sys;
    if (minor) *minor = 0;
    if (major) *major = 0;
    if (max) *max = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System12unloadPluginEj(void *sys, unsigned int handle) {
    (void)sys; (void)handle;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System13get3DSettingsEPfS1_S1_(void *sys, float *dopplerscale, float *distancefactor, float *rolloffscale) {
    (void)sys;
    if (dopplerscale) *dopplerscale = 1.0f;
    if (distancefactor) *distancefactor = 1.0f;
    if (rolloffscale) *rolloffscale = 1.0f;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System13getNumDriversEPi(void *sys, int *numdrivers) {
    (void)sys;
    if (numdrivers) *numdrivers = 1;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System13getNumPluginsE15FMOD_PLUGINTYPEPi(void *sys, int plugintype, int *numplugins) {
    (void)sys; (void)plugintype;
    if (numplugins) *numplugins = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System13getDriverInfoEiPciP9FMOD_GUIDPiP16FMOD_SPEAKERMODES4_(void *sys, int id, char *name, int namelen, void *guid, int *rate, void *speakermode, int *speakermodechannels) {
    (void)sys; (void)id; (void)guid;
    if (name) snprintf(name, namelen, "DSA Default");
    if (rate) *rate = 44100;
    if (speakermode) *(int*)speakermode = 0;
    if (speakermodechannels) *speakermodechannels = 2;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System13set3DSettingsEfff(void *sys, float dopplerscale, float distancefactor, float rolloffscale) {
    return DSA_System_Set3DSettings((DSA_System *)sys, dopplerscale, distancefactor, rolloffscale);
}
EXPORT int _ZN4FMOD6System13setFileSystemEPF11FMOD_RESULTPKcPjPPvS5_EPFS1_S5_S5_EPFS1_S5_S5_jS4_S5_EPFS1_S5_jS5_EPFS1_P18FMOD_ASYNCREADINFOS5_ESI_i(void *sys, void *open, void *close, void *read, void *seek, void *asyncread, void *asynccancel, int buffersize) {
    (void)sys; (void)open; (void)close; (void)read; (void)seek; (void)asyncread; (void)asynccancel; (void)buffersize;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System15createDSPByTypeE13FMOD_DSP_TYPEPPNS_3DSPE(void *sys, int type, void **dsp) {
    return DSA_System_CreateDSPByType((DSA_System *)sys, type, (DSA_DSP **)dsp);
}
EXPORT int _ZN4FMOD6System15getPluginHandleE15FMOD_PLUGINTYPEiPj(void *sys, int ptype, int index, unsigned int *handle) {
    (void)sys; (void)ptype; (void)index;
    if (handle) *handle = (unsigned int)(index + 1);
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System16getDSPBufferSizeEPjPi(void *sys, unsigned int *length, int *numbuffers) {
    (void)sys;
    if (length) *length = 1024;
    if (numbuffers) *numbuffers = 4;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System16getDSPInfoByTypeE13FMOD_DSP_TYPEPPK20FMOD_DSP_DESCRIPTION(void *sys, int type, const void **desc) {
    (void)sys; (void)type;
    if (desc) *desc = NULL;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System16setDSPBufferSizeEji(void *sys, unsigned int length, int numbuffers) {
    return DSA_System_SetDSPBufferSize((DSA_System *)sys, length, numbuffers);
}
EXPORT int _ZN4FMOD6System17createDSPByPluginEjPPNS_3DSPE(void *sys, unsigned int handle, void **dsp) {
    (void)sys; (void)handle; (void)dsp;
    return FMOD_ERR_NOTIMPLEMENTED;
}
EXPORT int _ZN4FMOD6System17getSoftwareFormatEPiP16FMOD_SPEAKERMODES1_(void *sys, int *samplerate, void *speakermode, int *numrawspeakers) {
    return DSA_System_GetSoftwareFormat((DSA_System *)sys, samplerate, (int*)speakermode, numrawspeakers);
}
EXPORT int _ZN4FMOD6System17set3DNumListenersEi(void *sys, int numlisteners) {
    (void)sys; (void)numlisteners;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System17setSoftwareFormatEi16FMOD_SPEAKERMODEi(void *sys, int samplerate, int speakermode, int numrawspeakers) {
    return DSA_System_SetSoftwareFormat((DSA_System *)sys, samplerate, speakermode, numrawspeakers);
}
EXPORT int _ZN4FMOD6System18createChannelGroupEPKcPPNS_12ChannelGroupE(void *sys, const char *name, void **group) {
    /* DSA doesn't have CreateChannelGroup yet, create SoundGroup as proxy */
    return DSA_System_CreateSoundGroup((DSA_System *)sys, name, (DSA_SoundGroup **)group);
}
EXPORT int _ZN4FMOD6System18getDSPInfoByPluginEjPPK20FMOD_DSP_DESCRIPTION(void *sys, unsigned int handle, const void **desc) {
    (void)sys; (void)handle;
    if (desc) *desc = NULL;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System19getAdvancedSettingsEP21FMOD_ADVANCEDSETTINGS(void *sys, void *settings) {
    (void)sys; (void)settings;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System19getDefaultMixMatrixE16FMOD_SPEAKERMODES1_Pfi(void *sys, int sourcespeakermode, int targetspeakermode, float *matrix, int channelhop) {
    (void)sys; (void)sourcespeakermode; (void)targetspeakermode; (void)matrix; (void)channelhop;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System19setAdvancedSettingsEP21FMOD_ADVANCEDSETTINGS(void *sys, const void *settings) {
    return DSA_System_SetAdvancedSettings((DSA_System *)sys, (const DSA_AdvancedSettings *)settings);
}
EXPORT int _ZN4FMOD6System10loadPluginEPKcPjj(void *sys, const char *filename, unsigned int *handle, unsigned int type) {
    (void)sys; (void)filename; (void)type;
    if (handle) *handle = 1;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System9getDriverEPi(void *sys, int *driver) {
    (void)sys;
    if (driver) *driver = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System9setDriverEi(void *sys, int driver) {
    (void)sys; (void)driver;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System10getVersionEPjS1_(void *sys, unsigned int *version, unsigned int *build) {
    if (version) *version = 0x00020306;
    if (build) *build = 0;
    (void)sys;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System23get3DListenerAttributesEiP11FMOD_VECTORS2_S2_S2_(void *sys, int listener, void *pos, void *vel, void *forward, void *up) {
    (void)sys; (void)listener; (void)pos; (void)vel; (void)forward; (void)up;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System23set3DListenerAttributesEiPK11FMOD_VECTORS3_S3_S3_(void *sys, int listener, const void *pos, const void *vel, const void *forward, const void *up) {
    (void)sys; (void)listener; (void)pos; (void)vel; (void)forward; (void)up;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System21getMasterChannelGroupEPPNS_12ChannelGroupE(void *sys, void **group) {
    return DSA_System_GetMasterChannelGroup((DSA_System *)sys, (DSA_ChannelGroup **)group);
}
EXPORT int _ZN4FMOD6System22getSpeakerModeChannelsE16FMOD_SPEAKERMODEPi(void *sys, int speakermode, int *channels) {
    (void)sys; (void)speakermode;
    if (channels) *channels = 2;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System24attachChannelGroupToPortE14FMOD_PORT_TYPEyPNS_12ChannelGroupEb(void *sys, int porttype, unsigned long long port, void *group, int pass) {
    (void)sys; (void)porttype; (void)port; (void)group; (void)pass;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System26detachChannelGroupFromPortEPNS_12ChannelGroupE(void *sys, void *group) {
    (void)sys; (void)group;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System7playDSPEPNS_3DSPEPNS_12ChannelGroupEbPPNS_7ChannelE(void *sys, void *dsp, void *group, int paused, void **channel) {
    (void)sys; (void)dsp; (void)group; (void)paused; (void)channel;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System9createDSPEPK20FMOD_DSP_DESCRIPTIONPPNS_3DSPE(void *sys, const void *desc, void **dsp) {
    (void)sys; (void)desc; (void)dsp;
    return FMOD_ERR_NOTIMPLEMENTED;
}
EXPORT int _ZN4FMOD6System9getOutputEP15FMOD_OUTPUTTYPE(void *sys, int *outputtype) {
    (void)sys;
    if (outputtype) *outputtype = 1;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD6System9setOutputE15FMOD_OUTPUTTYPE(void *sys, int outputtype) {
    (void)sys; (void)outputtype;
    return FMOD_OK;
}

/* =====================================================
 * FMOD::Sound
 * ===================================================== */
EXPORT int _ZN4FMOD5Sound7releaseEv(void *sound) {
    return DSA_Sound_Release((DSA_Sound *)sound);
}
EXPORT int _ZN4FMOD5Sound7getModeEPj(void *sound, unsigned int *mode) {
    int m = 0;
    int r = DSA_Sound_GetMode((DSA_Sound *)sound, &m);
    if (mode) *mode = (unsigned int)m;
    return r;
}
EXPORT int _ZN4FMOD5Sound7setModeEj(void *sound, unsigned int mode) {
    return DSA_Sound_SetMode((DSA_Sound *)sound, (int)mode);
}
EXPORT int _ZN4FMOD5Sound9getLengthEPjj(void *sound, unsigned int *length, int lengthunit) {
    return DSA_Sound_GetLength((DSA_Sound *)sound, length, lengthunit);
}
EXPORT int _ZN4FMOD5Sound9getFormatEP15FMOD_SOUND_TYPEP17FMOD_SOUND_FORMATPiS5_(void *sound, int *type, int *format, int *channels, int *bits) {
    DSA_SoundInfo info;
    memset(&info, 0, sizeof(info));
    int r = DSA_Sound_GetInfo((DSA_Sound *)sound, &info);
    if (type) *type = 0; /* FMOD_SOUND_TYPE_UNKNOWN */
    if (format) *format = info.bits_per_sample == 16 ? 2 : 1; /* PCM16 or PCM8 */
    if (channels) *channels = info.channels;
    if (bits) *bits = info.bits_per_sample;
    return r;
}
EXPORT int _ZN4FMOD5Sound11getDefaultsEPfPi(void *sound, float *frequency, int *priority) {
    (void)sound;
    if (frequency) *frequency = 44100.0f;
    if (priority) *priority = 128;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD5Sound11setDefaultsEfi(void *sound, float frequency, int priority) {
    (void)sound; (void)frequency; (void)priority;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD5Sound11getSubSoundEiPPS0_(void *sound, int index, void **subsound) {
    (void)sound; (void)index; (void)subsound;
    return FMOD_ERR_BADCOMMAND;
}
EXPORT int _ZN4FMOD5Sound12getOpenStateEP14FMOD_OPENSTATEPjPbS4_(void *sound, int *openstate, unsigned int *percentbuffered, int *starving, int *diskbusy) {
    (void)sound;
    if (openstate) *openstate = 2; /* FMOD_OPENSTATE_READY */
    if (percentbuffered) *percentbuffered = 100;
    if (starving) *starving = 0;
    if (diskbusy) *diskbusy = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD5Sound13getLoopPointsEPjjS1_j(void *sound, unsigned int *start, int unit_start, unsigned int *end, int unit_end) {
    (void)sound;
    if (start) *start = 0;
    if (end) *end = 0;
    (void)unit_start; (void)unit_end;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD5Sound15getNumSubSoundsEPi(void *sound, int *num) {
    (void)sound;
    if (num) *num = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD5Sound11setUserDataEPv(void *sound, void *userdata) {
    (void)sound; (void)userdata;
    return FMOD_OK;
}

/* =====================================================
 * FMOD::ChannelControl (base for Channel, ChannelGroup)
 * ===================================================== */
EXPORT int _ZN4FMOD14ChannelControl4stopEv(void *ctrl) {
    return DSA_Channel_Stop((DSA_Channel *)ctrl);
}
EXPORT int _ZN4FMOD14ChannelControl9setVolumeEf(void *ctrl, float volume) {
    return DSA_ChannelGroup_SetVolume((DSA_ChannelGroup *)ctrl, volume);
}
EXPORT int _ZN4FMOD14ChannelControl9getVolumeEPf(void *ctrl, float *volume) {
    return DSA_ChannelGroup_GetVolume((DSA_ChannelGroup *)ctrl, volume);
}
EXPORT int _ZN4FMOD14ChannelControl8setPitchEf(void *ctrl, float pitch) {
    return DSA_Channel_SetPitch((DSA_Channel *)ctrl, pitch);
}
EXPORT int _ZN4FMOD14ChannelControl8getPitchEPf(void *ctrl, float *pitch) {
    if (pitch) *pitch = 1.0f;
    (void)ctrl;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl9isPlayingEPb(void *ctrl, int *playing) {
    return DSA_Channel_IsPlaying((DSA_Channel *)ctrl, playing);
}
EXPORT int _ZN4FMOD14ChannelControl9setPausedEb(void *ctrl, int paused) {
    (void)ctrl; (void)paused;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl9getPausedEPb(void *ctrl, int *paused) {
    if (paused) *paused = 0;
    (void)ctrl;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl11setUserDataEPv(void *ctrl, void *userdata) {
    (void)ctrl; (void)userdata;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl7setMuteEb(void *ctrl, int mute) {
    (void)ctrl; (void)mute;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl7getModeEPj(void *ctrl, unsigned int *mode) {
    (void)ctrl;
    if (mode) *mode = DSA_DEFAULT;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl7setModeEj(void *ctrl, unsigned int mode) {
    (void)ctrl; (void)mode;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl13getAudibilityEPf(void *ctrl, float *audibility) {
    (void)ctrl;
    if (audibility) *audibility = 1.0f;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl13getVolumeRampEPb(void *ctrl, int *ramp) {
    (void)ctrl;
    if (ramp) *ramp = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl13setVolumeRampEb(void *ctrl, int ramp) {
    (void)ctrl; (void)ramp;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl15getSystemObjectEPPNS_6SystemE(void *ctrl, void **sys) {
    (void)ctrl;
    if (sys) *sys = (void*)1; /* dummy */
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl15set3DAttributesEPK11FMOD_VECTORS3_(void *ctrl, const void *pos, const void *vel) {
    (void)ctrl; (void)pos; (void)vel;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl17set3DDopplerLevelEf(void *ctrl, float level) {
    (void)ctrl; (void)level;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl19setReverbPropertiesEif(void *ctrl, int instance, float wet) {
    (void)ctrl; (void)instance; (void)wet;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl12addFadePointEyf(void *ctrl, unsigned long long dspclock, float volume) {
    (void)ctrl; (void)dspclock; (void)volume;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl12setMixMatrixEPfiii(void *ctrl, float *matrix, int outch, int inch, int channelhop) {
    (void)ctrl; (void)matrix; (void)outch; (void)inch; (void)channelhop;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl16removeFadePointsEyy(void *ctrl, unsigned long long start, unsigned long long end) {
    (void)ctrl; (void)start; (void)end;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl16setFadePointRampEyf(void *ctrl, unsigned long long dspclock, float volume) {
    (void)ctrl; (void)dspclock; (void)volume;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl11getDSPClockEPyS1_(void *ctrl, unsigned long long *dspclock, unsigned long long *parentclock) {
    (void)ctrl;
    if (dspclock) *dspclock = 0;
    if (parentclock) *parentclock = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl11getDSPIndexEPNS_3DSPEPi(void *ctrl, void *dsp, int *index) {
    (void)ctrl; (void)dsp; (void)index;
    if (index) *index = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl6addDSPEiPNS_3DSPE(void *ctrl, int index, void *dsp) {
    (void)index;
    DSA_DSPConnection *conn = NULL;
    /* Try channel first, then channelgroup */
    int r = DSA_Channel_AddDSP((DSA_Channel *)ctrl, (DSA_DSP *)dsp, &conn);
    if (r != 0) r = DSA_ChannelGroup_AddDSP((DSA_ChannelGroup *)ctrl, (DSA_DSP *)dsp, &conn);
    return r;
}
EXPORT int _ZN4FMOD14ChannelControl6getDSPEiPPNS_3DSPE(void *ctrl, int index, void **dsp) {
    (void)ctrl; (void)index; (void)dsp;
    return FMOD_ERR_BADCOMMAND;
}
EXPORT int _ZN4FMOD14ChannelControl9removeDSPEPNS_3DSPE(void *ctrl, void *dsp) {
    (void)ctrl; return DSA_DSP_Remove((DSA_DSP *)dsp);
}
EXPORT int _ZN4FMOD14ChannelControl8getDelayEPyS1_Pb(void *ctrl, unsigned long long *dspclock, unsigned long long *parentclock, int *sequential) {
    (void)ctrl;
    if (dspclock) *dspclock = 0;
    if (parentclock) *parentclock = 0;
    if (sequential) *sequential = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD14ChannelControl8setDelayEyyb(void *ctrl, unsigned long long dspclock, unsigned long long parentclock, int sequential) {
    (void)ctrl; (void)dspclock; (void)parentclock; (void)sequential;
    return FMOD_OK;
}

/* =====================================================
 * FMOD::ChannelGroup (extends ChannelControl)
 * ===================================================== */
EXPORT int _ZN4FMOD12ChannelGroup7releaseEv(void *group) {
    return DSA_ChannelGroup_Release((DSA_ChannelGroup *)group);
}
EXPORT int _ZN4FMOD12ChannelGroup8addGroupEPS0_bPPNS_13DSPConnectionE(void *group, void *child, int paused, void **connection) {
    (void)group; (void)child; (void)paused; (void)connection;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD12ChannelGroup8getGroupEiPPS0_(void *group, int index, void **subgroup) {
    (void)group; (void)index; (void)subgroup;
    return FMOD_ERR_BADCOMMAND;
}
EXPORT int _ZN4FMOD12ChannelGroup12getNumGroupsEPi(void *group, int *num) {
    (void)group;
    if (num) *num = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD12ChannelGroup14getNumChannelsEPi(void *group, int *num) {
    return DSA_ChannelGroup_GetNumChannels((DSA_ChannelGroup *)group, num);
}
EXPORT int _ZN4FMOD12ChannelGroup10getChannelEiPPNS_7ChannelE(void *group, int index, void **channel) {
    return DSA_ChannelGroup_GetChannel((DSA_ChannelGroup *)group, index, (DSA_Channel **)channel);
}
EXPORT int _ZN4FMOD12ChannelGroup14getParentGroupEPPS0_(void *group, void **parent) {
    (void)group;
    if (parent) *parent = NULL;
    return FMOD_OK;
}

/* =====================================================
 * FMOD::Channel (extends ChannelControl)
 * ===================================================== */
EXPORT int _ZN4FMOD7Channel11getPositionEPjj(void *channel, unsigned int *position, int timeunit) {
    return DSA_Channel_GetPosition((DSA_Channel *)channel, position, timeunit);
}
EXPORT int _ZN4FMOD7Channel11setPositionEjj(void *channel, unsigned int position, int timeunit) {
    return DSA_Channel_SetPosition((DSA_Channel *)channel, position, timeunit);
}
EXPORT int _ZN4FMOD7Channel11setPriorityEi(void *channel, int priority) {
    (void)channel; (void)priority;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD7Channel12getLoopCountEPi(void *channel, int *loopcount) {
    (void)channel;
    if (loopcount) *loopcount = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD7Channel12setLoopCountEi(void *channel, int loopcount) {
    (void)channel; (void)loopcount;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD7Channel15getCurrentSoundEPPNS_5SoundE(void *channel, void **sound) {
    return DSA_Channel_GetCurrentSound((DSA_Channel *)channel, (DSA_Sound **)sound);
}
EXPORT int _ZN4FMOD7Channel15setChannelGroupEPNS_12ChannelGroupE(void *channel, void *group) {
    (void)channel; (void)group;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD7Channel9isVirtualEPb(void *channel, int *isvirtual) {
    (void)channel;
    if (isvirtual) *isvirtual = 0;
    return FMOD_OK;
}

/* =====================================================
 * FMOD::DSP
 * ===================================================== */
EXPORT int _ZN4FMOD3DSP7releaseEv(void *dsp) {
    return DSA_DSP_Release((DSA_DSP *)dsp);
}
EXPORT int _ZN4FMOD3DSP9setActiveEb(void *dsp, int active) {
    return DSA_DSP_SetActive((DSA_DSP *)dsp, active);
}
EXPORT int _ZN4FMOD3DSP9setBypassEb(void *dsp, int bypass) {
    (void)dsp; (void)bypass;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP5resetEv(void *dsp) {
    (void)dsp;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP7getIdleEPb(void *dsp, int *idle) {
    (void)dsp;
    if (idle) *idle = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP7getTypeEP13FMOD_DSP_TYPE(void *dsp, int *type) {
    (void)dsp;
    if (type) *type = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP8addInputEPS0_PPNS_13DSPConnectionE23FMOD_DSPCONNECTION_TYPE(void *dsp, void *input, void **connection, int conn_type) {
    (void)conn_type;
    return DSA_DSP_AddDSP((DSA_DSP *)dsp, (DSA_DSP *)input, (DSA_DSPConnection **)connection);
}
EXPORT int _ZN4FMOD3DSP14disconnectFromEPS0_PNS_13DSPConnectionE(void *dsp, void *input, void *conn) {
    (void)dsp; (void)input; (void)conn;
    return DSA_DSP_Remove((DSA_DSP *)dsp);
}
EXPORT int _ZN4FMOD3DSP11getCPUUsageEPjS1_(void *dsp, unsigned int *callbacks, unsigned int *signal) {
    (void)dsp;
    if (callbacks) *callbacks = 0;
    if (signal) *signal = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP11getUserDataEPPv(void *dsp, void **userdata) {
    (void)dsp;
    if (userdata) *userdata = NULL;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP11setCallbackEPF11FMOD_RESULTP8FMOD_DSP22FMOD_DSP_CALLBACK_TYPEPvE(void *dsp, void *callback) {
    (void)dsp; (void)callback;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP12getWetDryMixEPfS1_S1_(void *dsp, float *prewet, float *postwet, float *dry) {
    (void)dsp;
    if (prewet) *prewet = 1.0f;
    if (postwet) *postwet = 1.0f;
    if (dry) *dry = 0.0f;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP12setWetDryMixEfff(void *dsp, float prewet, float postwet, float dry) {
    (void)dsp; (void)prewet; (void)postwet; (void)dry;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP15getMeteringInfoEP22FMOD_DSP_METERING_INFOS2_(void *dsp, void *input, void *output) {
    (void)dsp; (void)input; (void)output;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP15getParameterIntEiPiPci(void *dsp, int index, int *value, char *valuestr, int valuelen) {
    (void)dsp; (void)index; (void)valuestr; (void)valuelen;
    if (value) *value = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP15setParameterIntEii(void *dsp, int index, int value) {
    (void)dsp; (void)index; (void)value;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP16getNumParametersEPi(void *dsp, int *num) {
    (void)dsp;
    if (num) *num = 0;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP16getParameterDataEiPPvPjPci(void *dsp, int index, void **data, unsigned int *size, char *valuestr, int valuelen) {
    (void)dsp; (void)index; (void)data; (void)size; (void)valuestr; (void)valuelen;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP16getParameterInfoEiPP23FMOD_DSP_PARAMETER_DESC(void *dsp, int index, void **desc) {
    (void)dsp; (void)index;
    if (desc) *desc = NULL;
    return FMOD_ERR_BADCOMMAND;
}
EXPORT int _ZN4FMOD3DSP16setChannelFormatEji16FMOD_SPEAKERMODE(void *dsp, unsigned int inmask, int inchannels, int speakermode) {
    (void)dsp; (void)inmask; (void)inchannels; (void)speakermode;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP16setParameterBoolEib(void *dsp, int index, int value) {
    (void)dsp; (void)index; (void)value;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP16setParameterDataEiPvj(void *dsp, int index, void *data, unsigned int size) {
    (void)dsp; (void)index; (void)data; (void)size;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP17getParameterFloatEiPfPci(void *dsp, int index, float *value, char *valuestr, int valuelen) {
    (void)dsp; (void)index; (void)valuestr; (void)valuelen;
    if (value) *value = 0.0f;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP17setParameterFloatEif(void *dsp, int index, float value) {
    return DSA_DSP_SetParameterFloat((DSA_DSP *)dsp, index, value);
}
EXPORT int _ZN4FMOD3DSP18setMeteringEnabledEbb(void *dsp, int input, int output) {
    (void)dsp; (void)input; (void)output;
    return FMOD_OK;
}
EXPORT int _ZN4FMOD3DSP15getSystemObjectEPPNS_6SystemE(void *dsp, void **sys) {
    (void)dsp;
    if (sys) *sys = (void*)1;
    return FMOD_OK;
}

/* =====================================================
 * FMOD::DSPConnection
 * ===================================================== */
EXPORT int _ZN4FMOD13DSPConnection6setMixEf(void *conn, float mix) {
    return DSA_DSPConnection_SetMix((DSA_DSPConnection *)conn, mix);
}
EXPORT int _ZN4FMOD13DSPConnection12setMixMatrixEPfiii(void *conn, float *matrix, int outch, int inch, int channelhop) {
    (void)conn; (void)matrix; (void)outch; (void)inch; (void)channelhop;
    return FMOD_OK;
}
