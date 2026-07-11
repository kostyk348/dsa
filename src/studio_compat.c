/* FMOD Studio API compatibility layer — exports FMOD_Studio_* symbols
 * 
 * The Studio API wraps a Core system. When FMOD_Studio_System_Create is called,
 * we create a DSA system internally and track it.
 *
 * LD_PRELOAD this as libfmodstudio.so alongside libfmod.so. */

#include "dsa_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define EXPORT __attribute__((visibility("default")))

/* Track the mapping between Studio handles and Core systems */
typedef struct {
    void *studio_handle;
    void *core_system;
    int   refcount;
} StudioMapping;

#define MAX_STUDIO_SYSTEMS 8
static StudioMapping g_studio_map[MAX_STUDIO_SYSTEMS];
static int g_studio_count = 0;

/* For FMOD_RESULT compatibility */
#define FMOD_OK 0
#define FMOD_ERR_INVALID_HANDLE 30
#define FMOD_ERR_INTERNAL 28

/* Studio EventInstance struct — tracks association to Core sound */
typedef struct {
    void     *studio_system;
    DSA_System *core_system;
    DSA_Sound  *sound;
} DSA_StudioInstance;

EXPORT int FMOD_Studio_System_Create(void **studio, unsigned int headerversion) {
    if (!studio) return 1;
    (void)headerversion;

    /* Create a DSA system */
    DSA_System *core = NULL;
    int r = DSA_System_Create(&core, headerversion);
    if (r != 0) return r;

    /* Create the Studio handle (just an alloc'd int to have a unique pointer) */
    void *handle = malloc(8);
    if (!handle) { DSA_System_Release(core); return 1; }

    /* Store in mapping table */
    if (g_studio_count < MAX_STUDIO_SYSTEMS) {
        g_studio_map[g_studio_count].studio_handle = handle;
        g_studio_map[g_studio_count].core_system = core;
        g_studio_map[g_studio_count].refcount = 1;
        g_studio_count++;
    }

    *studio = handle;
    dsa_log( "[dsa-studio] Create: studio=%p core=%p\n", handle, (void*)core);
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_Release(void *studio) {
    if (!studio) return 1;

    /* Find mapping and release core system */
    for (int i = 0; i < g_studio_count; i++) {
        if (g_studio_map[i].studio_handle == studio) {
            if (g_studio_map[i].refcount > 1) {
                g_studio_map[i].refcount--;
            } else {
                DSA_System_Release((DSA_System *)g_studio_map[i].core_system);
                free(g_studio_map[i].studio_handle);
                g_studio_map[i].studio_handle = NULL;
                g_studio_map[i].core_system = NULL;
            }
            return FMOD_OK;
        }
    }
    return 1;
}

EXPORT int FMOD_Studio_System_GetCoreSystem(void *studio, void **core) {
    if (!studio || !core) return 1;

    for (int i = 0; i < g_studio_count; i++) {
        if (g_studio_map[i].studio_handle == studio) {
            *core = g_studio_map[i].core_system;
            return FMOD_OK;
        }
    }
    return 1;
}

EXPORT int FMOD_Studio_System_FlushCommands(void *studio) {
    (void)studio;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_FlushSampleLoading(void *studio) {
    (void)studio;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetAdvancedSettings(void *studio, void *settings) {
    (void)studio; (void)settings;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetBank(void *studio, const char *path, void **bank) {
    (void)studio; (void)path; (void)bank;
    return 18; /* FMOD_ERR_FILE_NOTFOUND — no banks loaded */
}

EXPORT int FMOD_Studio_System_GetBankByID(void *studio, const void *guid, void **bank) {
    (void)studio; (void)guid; (void)bank;
    return 18;
}

EXPORT int FMOD_Studio_System_GetBankCount(void *studio, int *count) {
    (void)studio;
    if (count) *count = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetBankList(void *studio, void **banks, int capacity, int *count) {
    (void)studio; (void)banks; (void)capacity;
    if (count) *count = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetBufferUsage(void *studio, void *usage) {
    (void)studio; (void)usage;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetBus(void *studio, const char *path, void **bus) {
    (void)studio; (void)path; (void)bus;
    return 30; /* FMOD_ERR_INVALID_HANDLE — no buses */
}

EXPORT int FMOD_Studio_System_GetCPUUsage(void *studio, void *usage) {
    (void)studio; (void)usage;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetEvent(void *studio, const char *path, void **event) {
    (void)studio; (void)path; (void)event;
    return 30;
}

EXPORT int FMOD_Studio_System_GetEventByID(void *studio, const void *guid, void **event) {
    (void)studio; (void)guid; (void)event;
    return 30;
}

EXPORT int FMOD_Studio_System_GetParameterByID(void *studio, const void *id, void *param) {
    (void)studio; (void)id; (void)param;
    return 30;
}

EXPORT int FMOD_Studio_System_GetParameterByName(void *studio, const char *name, void *param) {
    (void)studio; (void)name; (void)param;
    return 30;
}

EXPORT int FMOD_Studio_System_GetParameterDescriptionByID(void *studio, const void *id, void *desc) {
    (void)studio; (void)id; (void)desc;
    return 30;
}

EXPORT int FMOD_Studio_System_GetParameterDescriptionByName(void *studio, const char *name, void *desc) {
    (void)studio; (void)name; (void)desc;
    return 30;
}

EXPORT int FMOD_Studio_System_GetParameterDescriptionCount(void *studio, int *count) {
    (void)studio;
    if (count) *count = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetPath(void *studio, const void *id, char *path, int size, int *ret_size) {
    (void)studio; (void)id; (void)path; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetSoundInfo(void *studio, const char *key, void *info) {
    (void)studio; (void)key; (void)info;
    return 18;
}

EXPORT int FMOD_Studio_System_GetVCAName(void *studio, void *vca, char *name, int size, int *ret_size) {
    (void)studio; (void)vca; (void)name; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetVCACount(void *studio, int *count) {
    (void)studio;
    if (count) *count = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetVCAList(void *studio, void **vca, int capacity, int *count) {
    (void)studio; (void)vca; (void)capacity;
    if (count) *count = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_LookupID(void *studio, const char *path, void *id) {
    (void)studio; (void)path; (void)id;
    return 30;
}

EXPORT int FMOD_Studio_System_LoadBankFile(void *studio, const char *file, int flags, void **bank) {
    (void)studio; (void)file; (void)flags; (void)bank;
    return 54; /* FMOD_ERR_NOTIMPLEMENTED */
}

EXPORT int FMOD_Studio_System_LoadBankMemory(void *studio, const void *data, int size, int mode, int flags, void **bank) {
    (void)studio; (void)data; (void)size; (void)mode; (void)flags; (void)bank;
    return 54;
}

EXPORT int FMOD_Studio_System_LoadBankCustom(void *studio, const void *desc, int flags, void **bank) {
    (void)studio; (void)desc; (void)flags; (void)bank;
    return 54;
}

EXPORT int FMOD_Studio_System_GetLowLevelPath(void *studio, char *path, int size, int *ret_size) {
    (void)studio; (void)path; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetParameterCount(void *studio, int *count) {
    (void)studio;
    if (count) *count = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_GetParameterValue(void *studio, const void *id, float *value, float *final) {
    (void)studio; (void)id;
    if (value) *value = 0;
    if (final) *final = 0;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_SetCallback(void *studio, void *callback, int mask) {
    (void)studio; (void)callback; (void)mask;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_SetListenerAttributes(void *studio, int listener, const void *attrs, void *attenuation) {
    (void)studio; (void)listener; (void)attrs; (void)attenuation;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_SetParameterByID(void *studio, const void *id, float value, int ignoreseekspeed) {
    (void)studio; (void)id; (void)value; (void)ignoreseekspeed;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_SetParameterByName(void *studio, const char *name, float value, int ignoreseekspeed) {
    (void)studio; (void)name; (void)value; (void)ignoreseekspeed;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_Start(void *studio) {
    (void)studio;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_Stop(void *studio) {
    (void)studio;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_Update(void *studio) {
    (void)studio;
    return FMOD_OK;
}

EXPORT int FMOD_Studio_System_IsValid(void *studio) {
    if (!studio) return 0;

    for (int i = 0; i < g_studio_count; i++) {
        if (g_studio_map[i].studio_handle == studio) return 1;
    }
    return 0;
}

/* =====================================================
 * Studio C++ ABI symbols
 * ===================================================== */
#define FMOD_STUDIO_STOP_ALLOWFADEOUT 1
#define FMOD_STUDIO_LOAD_BANK_NORMAL  0

/* FMOD::Studio::System::create */
EXPORT int _ZN4FMOD6Studio6System6createEPPS1_j(void **studio, unsigned int headerversion) {
    return FMOD_Studio_System_Create(studio, headerversion);
}
/* FMOD::Studio::System::release */
EXPORT int _ZN4FMOD6Studio6System7releaseEv(void *studio) {
    return FMOD_Studio_System_Release(studio);
}
/* FMOD::Studio::System::initialize */
EXPORT int _ZN4FMOD6Studio6System10initializeEijjPv(void *studio, int maxchannels, unsigned int flags, unsigned int studioflags, void *extradriverdata) {
    (void)studioflags;
    /* Get core system and init it */
    void *core = NULL;
    int r = FMOD_Studio_System_GetCoreSystem(studio, &core);
    if (r != 0) return r;
    return DSA_System_Init((DSA_System *)core, maxchannels, flags, extradriverdata);
}
/* FMOD::Studio::System::update */
EXPORT int _ZN4FMOD6Studio6System6updateEv(void *studio) {
    return FMOD_Studio_System_Update(studio);
}
/* FMOD::Studio::System::loadBankFile */
EXPORT int _ZN4FMOD6Studio6System12loadBankFileEPKcjPPNS0_4BankE(void *studio, const char *path, int flags, void **bank) {
    return FMOD_Studio_System_LoadBankFile(studio, path, flags, bank);
}
/* FMOD::Studio::System::unloadAll */
EXPORT int _ZN4FMOD6Studio6System9unloadAllEv(void *studio) {
    (void)studio;
    return FMOD_OK;
}
/* FMOD::Studio::System::flushSampleLoading */
EXPORT int _ZN4FMOD6Studio6System18flushSampleLoadingEv(void *studio) {
    return FMOD_Studio_System_FlushSampleLoading(studio);
}
/* FMOD::Studio::System::setListenerAttributes */
EXPORT int _ZN4FMOD6Studio6System21setListenerAttributesEiPK18FMOD_3D_ATTRIBUTESPK11FMOD_VECTOR(void *studio, int listener, const void *attrs, const void *attenuation) {
    (void)attenuation;
    return FMOD_Studio_System_SetListenerAttributes(studio, listener, attrs, NULL);
}
/* FMOD::Studio::System::setNumListeners */
EXPORT int _ZN4FMOD6Studio6System15setNumListenersEi(void *studio, int num) {
    (void)studio; (void)num;
    return FMOD_OK;
}
/* FMOD::Studio::System::getListenerWeight */
EXPORT int _ZN4FMOD6Studio6System17getListenerWeightEiPf(void *studio, int listener, float *weight) {
    (void)studio; (void)listener;
    if (weight) *weight = 1.0f;
    return FMOD_OK;
}
/* FMOD::Studio::System::setListenerWeight */
EXPORT int _ZN4FMOD6Studio6System17setListenerWeightEif(void *studio, int listener, float weight) {
    (void)studio; (void)listener; (void)weight;
    return FMOD_OK;
}
/* FMOD::Studio::System::setParameterByID */
EXPORT int _ZN4FMOD6Studio6System16setParameterByIDE24FMOD_STUDIO_PARAMETER_IDfb(void *studio, const void *id, float value, int ignoreseekspeed) {
    return FMOD_Studio_System_SetParameterByID(studio, id, value, ignoreseekspeed);
}
/* FMOD::Studio::System::setParameterByName */
EXPORT int _ZN4FMOD6Studio6System18setParameterByNameEPKcfb(void *studio, const char *name, float value, int ignoreseekspeed) {
    return FMOD_Studio_System_SetParameterByName(studio, name, value, ignoreseekspeed);
}
/* FMOD::Studio::System::setParameterByIDWithLabel */
EXPORT int _ZN4FMOD6Studio6System25setParameterByIDWithLabelE24FMOD_STUDIO_PARAMETER_IDPKcb(void *studio, const void *id, const char *label, int ignoreseekspeed) {
    (void)studio; (void)id; (void)label; (void)ignoreseekspeed;
    return FMOD_OK;
}
/* FMOD::Studio::System::setParameterByNameWithLabel */
EXPORT int _ZN4FMOD6Studio6System27setParameterByNameWithLabelEPKcS3_b(void *studio, const char *name, const char *label, int ignoreseekspeed) {
    (void)studio; (void)name; (void)label; (void)ignoreseekspeed;
    return FMOD_OK;
}

/* FMOD::Studio::EventDescription::loadSampleData */
EXPORT int _ZN4FMOD6Studio16EventDescription14loadSampleDataEv(void *desc) {
    (void)desc;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::unloadSampleData */
EXPORT int _ZN4FMOD6Studio16EventDescription16unloadSampleDataEv(void *desc) {
    (void)desc;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::releaseAllInstances */
EXPORT int _ZN4FMOD6Studio16EventDescription19releaseAllInstancesEv(void *desc) {
    (void)desc;
    return FMOD_OK;
}

/* FMOD::Studio::EventInstance::start */
EXPORT int _ZN4FMOD6Studio13EventInstance5startEv(void *inst) {
    if (!inst) return FMOD_ERR_INVALID_HANDLE;
    DSA_StudioInstance *si = (DSA_StudioInstance *)inst;

    if (si->core_system && si->sound) {
        DSA_Channel *ch = NULL;
        int r = DSA_System_PlaySound(si->core_system, si->sound, NULL, 0, &ch);
        dsa_log( "[dsa-studio] start: inst=%p sound=%p → ch=%p ret=%d\n",
                inst, (void*)si->sound, (void*)ch, r);
    } else if (si->core_system) {
        /* No associated sound — try playing last created Core sound */
        DSA_Sound *last = NULL;
        if (DSA_System_GetLastSound(si->core_system, &last) == 0 && last) {
            DSA_Channel *ch = NULL;
            DSA_System_PlaySound(si->core_system, last, NULL, 0, &ch);
            dsa_log( "[dsa-studio] start (fallback): last=%p ch=%p\n",
                    (void*)last, (void*)ch);
        }
    }
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::stop */
EXPORT int _ZN4FMOD6Studio13EventInstance4stopE21FMOD_STUDIO_STOP_MODE(void *inst, int stopmode) {
    (void)inst; (void)stopmode;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::release */
EXPORT int _ZN4FMOD6Studio13EventInstance7releaseEv(void *inst) {
    if (!inst) return FMOD_ERR_INVALID_HANDLE;
    free(inst);
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::keyOff */
EXPORT int _ZN4FMOD6Studio13EventInstance6keyOffEv(void *inst) {
    (void)inst;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setVolume */
EXPORT int _ZN4FMOD6Studio13EventInstance9setVolumeEf(void *inst, float volume) {
    (void)inst; (void)volume;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setPitch */
EXPORT int _ZN4FMOD6Studio13EventInstance8setPitchEf(void *inst, float pitch) {
    (void)inst; (void)pitch;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setPaused */
EXPORT int _ZN4FMOD6Studio13EventInstance9setPausedEb(void *inst, int paused) {
    (void)inst; (void)paused;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setTimelinePosition */
EXPORT int _ZN4FMOD6Studio13EventInstance19setTimelinePositionEi(void *inst, int position) {
    (void)inst; (void)position;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::set3DAttributes */
EXPORT int _ZN4FMOD6Studio13EventInstance15set3DAttributesEPK18FMOD_3D_ATTRIBUTES(void *inst, const void *attrs) {
    (void)inst; (void)attrs;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setListenerMask */
EXPORT int _ZN4FMOD6Studio13EventInstance15setListenerMaskEj(void *inst, unsigned int mask) {
    (void)inst; (void)mask;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setReverbLevel */
EXPORT int _ZN4FMOD6Studio13EventInstance14setReverbLevelEif(void *inst, int index, float level) {
    (void)inst; (void)index; (void)level;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setUserData */
EXPORT int _ZN4FMOD6Studio13EventInstance11setUserDataEPv(void *inst, void *userdata) {
    (void)inst; (void)userdata;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setCallback */
EXPORT int _ZN4FMOD6Studio13EventInstance11setCallbackEPF11FMOD_RESULTjP25FMOD_STUDIO_EVENTINSTANCEPvEj(void *inst, void *callback, unsigned int callbackmask) {
    (void)inst; (void)callback; (void)callbackmask;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setParameterByID */
EXPORT int _ZN4FMOD6Studio13EventInstance16setParameterByIDE24FMOD_STUDIO_PARAMETER_IDfb(void *inst, const void *id, float value, int ignoreseekspeed) {
    (void)inst; (void)id; (void)value; (void)ignoreseekspeed;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setParameterByName */
EXPORT int _ZN4FMOD6Studio13EventInstance18setParameterByNameEPKcfb(void *inst, const char *name, float value, int ignoreseekspeed) {
    (void)inst; (void)name; (void)value; (void)ignoreseekspeed;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setParameterByIDWithLabel */
EXPORT int _ZN4FMOD6Studio13EventInstance25setParameterByIDWithLabelE24FMOD_STUDIO_PARAMETER_IDPKcb(void *inst, const void *id, const char *label, int ignoreseekspeed) {
    (void)inst; (void)id; (void)label; (void)ignoreseekspeed;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::setParameterByNameWithLabel */
EXPORT int _ZN4FMOD6Studio13EventInstance27setParameterByNameWithLabelEPKcS3_b(void *inst, const char *name, const char *label, int ignoreseekspeed) {
    (void)inst; (void)name; (void)label; (void)ignoreseekspeed;
    return FMOD_OK;
}

/* FMOD::Studio::Bus::stopAllEvents */
EXPORT int _ZN4FMOD6Studio3Bus13stopAllEventsE21FMOD_STUDIO_STOP_MODE(void *bus, int stopmode) {
    (void)bus; (void)stopmode;
    return FMOD_OK;
}
/* FMOD::Studio::Bus::setVolume */
EXPORT int _ZN4FMOD6Studio3Bus9setVolumeEf(void *bus, float volume) {
    (void)bus; (void)volume;
    return FMOD_OK;
}
/* FMOD::Studio::Bus::setPaused */
EXPORT int _ZN4FMOD6Studio3Bus9setPausedEb(void *bus, int paused) {
    (void)bus; (void)paused;
    return FMOD_OK;
}
/* FMOD::Studio::Bus::setMute */
EXPORT int _ZN4FMOD6Studio3Bus7setMuteEb(void *bus, int mute) {
    (void)bus; (void)mute;
    return FMOD_OK;
}

/* FMOD::Studio::VCA::setVolume */
EXPORT int _ZN4FMOD6Studio3VCA9setVolumeEf(void *vca, float volume) {
    (void)vca; (void)volume;
    return FMOD_OK;
}

/* FMOD::Studio::Bank::unload */
EXPORT int _ZN4FMOD6Studio4Bank6unloadEv(void *bank) {
    (void)bank;
    return FMOD_OK;
}

/* =====================================================
 * Studio C++ ABI — Const methods (_ZNK prefix)
 * ===================================================== */
/* FMOD::Studio::System::getCoreSystem const */
EXPORT int _ZNK4FMOD6Studio6System13getCoreSystemEPPNS_6SystemE(void *studio, void **core) {
    return FMOD_Studio_System_GetCoreSystem(studio, core);
}
/* FMOD::Studio::System::getBus const */
EXPORT int _ZNK4FMOD6Studio6System6getBusEPKcPPNS0_3BusE(void *studio, const char *path, void **bus) {
    return FMOD_Studio_System_GetBus(studio, path, bus);
}
/* FMOD::Studio::System::getCPUUsage const */
EXPORT int _ZNK4FMOD6Studio6System11getCPUUsageEP21FMOD_STUDIO_CPU_USAGEP14FMOD_CPU_USAGE(void *studio, void *studiousage, void *coreusage) {
    (void)studio; (void)studiousage; (void)coreusage;
    return FMOD_OK;
}
/* FMOD::Studio::System::getSoundInfo const */
EXPORT int _ZNK4FMOD6Studio6System12getSoundInfoEPKcP22FMOD_STUDIO_SOUND_INFO(void *studio, const char *key, void *info) {
    return FMOD_Studio_System_GetSoundInfo(studio, key, info);
}
/* FMOD::Studio::System::getParameterByID const */
EXPORT int _ZNK4FMOD6Studio6System16getParameterByIDE24FMOD_STUDIO_PARAMETER_IDPfS3_(void *studio, const void *id, float *value, float *finalval) {
    (void)studio; (void)id;
    if (value) *value = 0;
    if (finalval) *finalval = 0;
    return FMOD_OK;
}
/* FMOD::Studio::System::getParameterByName const */
EXPORT int _ZNK4FMOD6Studio6System18getParameterByNameEPKcPfS4_(void *studio, const char *name, float *value, float *finalval) {
    (void)studio; (void)name;
    if (value) *value = 0;
    if (finalval) *finalval = 0;
    return FMOD_OK;
}
/* FMOD::Studio::System::getListenerAttributes const */
EXPORT int _ZNK4FMOD6Studio6System21getListenerAttributesEiP18FMOD_3D_ATTRIBUTESP11FMOD_VECTOR(void *studio, int listener, void *attrs, void *attenuation) {
    (void)studio; (void)listener; (void)attrs; (void)attenuation;
    return FMOD_OK;
}
/* FMOD::Studio::System::getParameterDescriptionByID const */
EXPORT int _ZNK4FMOD6Studio6System27getParameterDescriptionByIDE24FMOD_STUDIO_PARAMETER_IDP33FMOD_STUDIO_PARAMETER_DESCRIPTION(void *studio, const void *id, void *desc) {
    (void)studio; (void)id; (void)desc;
    return DSA_ERR_INVALID_HANDLE;
}
/* FMOD::Studio::System::getParameterDescriptionList const */
EXPORT int _ZNK4FMOD6Studio6System27getParameterDescriptionListEP33FMOD_STUDIO_PARAMETER_DESCRIPTIONiPi(void *studio, void *desc, int capacity, int *count) {
    (void)studio; (void)desc; (void)capacity;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::System::getParameterDescriptionCount const */
EXPORT int _ZNK4FMOD6Studio6System28getParameterDescriptionCountEPi(void *studio, int *count) {
    return FMOD_Studio_System_GetParameterDescriptionCount(studio, count);
}
/* FMOD::Studio::System::getParameterDescriptionByName const */
EXPORT int _ZNK4FMOD6Studio6System29getParameterDescriptionByNameEPKcP33FMOD_STUDIO_PARAMETER_DESCRIPTION(void *studio, const char *name, void *desc) {
    return FMOD_Studio_System_GetParameterDescriptionByName(studio, name, desc);
}

/* FMOD::Studio::EventDescription::createInstance const */
EXPORT int _ZNK4FMOD6Studio16EventDescription14createInstanceEPPNS0_13EventInstanceE(void *desc, void **instance) {
    (void)desc;
    if (!instance) return FMOD_ERR_INVALID_HANDLE;

    /* Allocate a real instance struct that tracks the Core system */
    DSA_StudioInstance *inst = (DSA_StudioInstance *)calloc(1, sizeof(DSA_StudioInstance));
    if (!inst) return FMOD_ERR_INTERNAL;

    inst->studio_system = g_studio_count > 0 ? g_studio_map[0].studio_handle : NULL;
    inst->core_system = g_studio_count > 0 ? g_studio_map[0].core_system : NULL;

    /* Try to find the last created Core sound and associate it */
    if (inst->core_system) {
        DSA_System_GetLastSound(inst->core_system, &inst->sound);
    }

    dsa_log( "[dsa-studio] createInstance: desc=%p inst=%p core=%p sound=%p\n",
            desc, (void*)inst, (void*)inst->core_system, (void*)inst->sound);

    *instance = inst;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getLength const */
EXPORT int _ZNK4FMOD6Studio16EventDescription9getLengthEPi(void *desc, int *length) {
    (void)desc;
    if (length) *length = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::isOneshot const */
EXPORT int _ZNK4FMOD6Studio16EventDescription9isOneshotEPb(void *desc, int *oneshot) {
    (void)desc;
    if (oneshot) *oneshot = 1;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::isStream const */
EXPORT int _ZNK4FMOD6Studio16EventDescription8isStreamEPb(void *desc, int *isstream) {
    (void)desc;
    if (isstream) *isstream = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::is3D const */
EXPORT int _ZNK4FMOD6Studio16EventDescription4is3DEPb(void *desc, int *is3d) {
    (void)desc;
    if (is3d) *is3d = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::isSnapshot const */
EXPORT int _ZNK4FMOD6Studio16EventDescription10isSnapshotEPb(void *desc, int *snapshot) {
    (void)desc;
    if (snapshot) *snapshot = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::isValid const */
EXPORT int _ZNK4FMOD6Studio16EventDescription7isValidEv(void *desc) {
    return desc ? 1 : 0;
}
/* FMOD::Studio::EventDescription::getPath const */
EXPORT int _ZNK4FMOD6Studio16EventDescription7getPathEPciPi(void *desc, char *path, int size, int *ret_size) {
    (void)desc; (void)path; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getID const */
EXPORT int _ZNK4FMOD6Studio16EventDescription5getIDEP9FMOD_GUID(void *desc, void *guid) {
    (void)desc; (void)guid;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getSoundSize const */
EXPORT int _ZNK4FMOD6Studio16EventDescription12getSoundSizeEPf(void *desc, float *size) {
    (void)desc;
    if (size) *size = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getMinMaxDistance const */
EXPORT int _ZNK4FMOD6Studio16EventDescription17getMinMaxDistanceEPfS2_(void *desc, float *min, float *max) {
    (void)desc;
    if (min) *min = 0;
    if (max) *max = 100;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getInstanceCount const */
EXPORT int _ZNK4FMOD6Studio16EventDescription16getInstanceCountEPi(void *desc, int *count) {
    (void)desc;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getInstanceList const */
EXPORT int _ZNK4FMOD6Studio16EventDescription15getInstanceListEPPNS0_13EventInstanceEiPi(void *desc, void **list, int capacity, int *count) {
    (void)desc; (void)list; (void)capacity;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getSampleLoadingState const */
EXPORT int _ZNK4FMOD6Studio16EventDescription21getSampleLoadingStateEP25FMOD_STUDIO_LOADING_STATE(void *desc, int *state) {
    (void)desc;
    if (state) *state = 2; /* FMOD_STUDIO_LOADING_STATE_LOADED */
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::hasSustainPoint const */
EXPORT int _ZNK4FMOD6Studio16EventDescription15hasSustainPointEPb(void *desc, int *has) {
    (void)desc;
    if (has) *has = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getUserProperty const */
EXPORT int _ZNK4FMOD6Studio16EventDescription15getUserPropertyEPKcP25FMOD_STUDIO_USER_PROPERTY(void *desc, const char *name, void *prop) {
    (void)desc; (void)name; (void)prop;
    return DSA_ERR_INVALID_HANDLE;
}
/* FMOD::Studio::EventDescription::getUserPropertyCount const */
EXPORT int _ZNK4FMOD6Studio16EventDescription20getUserPropertyCountEPi(void *desc, int *count) {
    (void)desc;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getUserPropertyByIndex const */
EXPORT int _ZNK4FMOD6Studio16EventDescription22getUserPropertyByIndexEiP25FMOD_STUDIO_USER_PROPERTY(void *desc, int index, void *prop) {
    (void)desc; (void)index; (void)prop;
    return DSA_ERR_INVALID_HANDLE;
}
/* FMOD::Studio::EventDescription::getParameterDescriptionCount const */
EXPORT int _ZNK4FMOD6Studio16EventDescription28getParameterDescriptionCountEPi(void *desc, int *count) {
    (void)desc;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getParameterDescriptionByID const */
EXPORT int _ZNK4FMOD6Studio16EventDescription27getParameterDescriptionByIDE24FMOD_STUDIO_PARAMETER_IDP33FMOD_STUDIO_PARAMETER_DESCRIPTION(void *desc, const void *id, void *paramdesc) {
    (void)desc; (void)id; (void)paramdesc;
    return DSA_ERR_INVALID_HANDLE;
}
/* FMOD::Studio::EventDescription::getParameterDescriptionByName const */
EXPORT int _ZNK4FMOD6Studio16EventDescription29getParameterDescriptionByNameEPKcP33FMOD_STUDIO_PARAMETER_DESCRIPTION(void *desc, const char *name, void *paramdesc) {
    (void)desc; (void)name; (void)paramdesc;
    return DSA_ERR_INVALID_HANDLE;
}
/* FMOD::Studio::EventDescription::getParameterDescriptionByIndex const */
EXPORT int _ZNK4FMOD6Studio16EventDescription30getParameterDescriptionByIndexEiP33FMOD_STUDIO_PARAMETER_DESCRIPTION(void *desc, int index, void *paramdesc) {
    (void)desc; (void)index; (void)paramdesc;
    return DSA_ERR_INVALID_HANDLE;
}
/* FMOD::Studio::EventDescription::getParameterLabelByID const */
EXPORT int _ZNK4FMOD6Studio16EventDescription21getParameterLabelByIDE24FMOD_STUDIO_PARAMETER_IDiPciPi(void *desc, const void *id, int labelindex, char *label, int size, int *ret_size) {
    (void)desc; (void)id; (void)labelindex; (void)label; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getParameterLabelByName const */
EXPORT int _ZNK4FMOD6Studio16EventDescription23getParameterLabelByNameEPKciPciPi(void *desc, const char *name, int labelindex, char *label, int size, int *ret_size) {
    (void)desc; (void)name; (void)labelindex; (void)label; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventDescription::getParameterLabelByIndex const */
EXPORT int _ZNK4FMOD6Studio16EventDescription24getParameterLabelByIndexEiiPciPi(void *desc, int index, int labelindex, char *label, int size, int *ret_size) {
    (void)desc; (void)index; (void)labelindex; (void)label; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}

/* FMOD::Studio::EventInstance::isValid const */
EXPORT int _ZNK4FMOD6Studio13EventInstance7isValidEv(void *inst) {
    return inst ? 1 : 0;
}
/* FMOD::Studio::EventInstance::getVolume const */
EXPORT int _ZNK4FMOD6Studio13EventInstance9getVolumeEPfS2_(void *inst, float *volume, float *finalvol) {
    (void)inst;
    if (volume) *volume = 1.0f;
    if (finalvol) *finalvol = 1.0f;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getPitch const */
EXPORT int _ZNK4FMOD6Studio13EventInstance8getPitchEPfS2_(void *inst, float *pitch, float *finalpitch) {
    (void)inst;
    if (pitch) *pitch = 1.0f;
    if (finalpitch) *finalpitch = 1.0f;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getPaused const */
EXPORT int _ZNK4FMOD6Studio13EventInstance9getPausedEPb(void *inst, int *paused) {
    (void)inst;
    if (paused) *paused = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getTimelinePosition const */
EXPORT int _ZNK4FMOD6Studio13EventInstance19getTimelinePositionEPi(void *inst, int *position) {
    (void)inst;
    if (position) *position = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getPlaybackState const */
EXPORT int _ZNK4FMOD6Studio13EventInstance16getPlaybackStateEP26FMOD_STUDIO_PLAYBACK_STATE(void *inst, int *state) {
    (void)inst;
    if (state) *state = 2; /* FMOD_STUDIO_PLAYBACK_PLAYING */
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::get3DAttributes const */
EXPORT int _ZNK4FMOD6Studio13EventInstance15get3DAttributesEP18FMOD_3D_ATTRIBUTES(void *inst, void *attrs) {
    (void)inst; (void)attrs;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getListenerMask const */
EXPORT int _ZNK4FMOD6Studio13EventInstance15getListenerMaskEPj(void *inst, unsigned int *mask) {
    (void)inst;
    if (mask) *mask = 1;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getReverbLevel const */
EXPORT int _ZNK4FMOD6Studio13EventInstance14getReverbLevelEiPf(void *inst, int index, float *level) {
    (void)inst; (void)index;
    if (level) *level = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getUserData const */
EXPORT int _ZNK4FMOD6Studio13EventInstance11getUserDataEPPv(void *inst, void **userdata) {
    (void)inst;
    if (userdata) *userdata = NULL;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getParameterByID const */
EXPORT int _ZNK4FMOD6Studio13EventInstance16getParameterByIDE24FMOD_STUDIO_PARAMETER_IDPfS3_(void *inst, const void *id, float *value, float *finalval) {
    (void)inst; (void)id;
    if (value) *value = 0;
    if (finalval) *finalval = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::getParameterByName const */
EXPORT int _ZNK4FMOD6Studio13EventInstance18getParameterByNameEPKcPfS4_(void *inst, const char *name, float *value, float *finalval) {
    (void)inst; (void)name;
    if (value) *value = 0;
    if (finalval) *finalval = 0;
    return FMOD_OK;
}
/* FMOD::Studio::EventInstance::isVirtual const */
EXPORT int _ZNK4FMOD6Studio13EventInstance9isVirtualEPb(void *inst, int *isvirtual) {
    (void)inst;
    if (isvirtual) *isvirtual = 0;
    return FMOD_OK;
}

/* FMOD::Studio::Bus::isValid const */
EXPORT int _ZNK4FMOD6Studio3Bus7isValidEv(void *bus) {
    return bus ? 1 : 0;
}
/* FMOD::Studio::Bus::getID const */
EXPORT int _ZNK4FMOD6Studio3Bus5getIDEP9FMOD_GUID(void *bus, void *guid) {
    (void)bus; (void)guid;
    return FMOD_OK;
}
/* FMOD::Studio::Bus::getPath const */
EXPORT int _ZNK4FMOD6Studio3Bus7getPathEPciPi(void *bus, char *path, int size, int *ret_size) {
    (void)bus; (void)path; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bus::getVolume const */
EXPORT int _ZNK4FMOD6Studio3Bus9getVolumeEPfS2_(void *bus, float *volume, float *finalvol) {
    (void)bus;
    if (volume) *volume = 1.0f;
    if (finalvol) *finalvol = 1.0f;
    return FMOD_OK;
}
/* FMOD::Studio::Bus::getPaused const */
EXPORT int _ZNK4FMOD6Studio3Bus9getPausedEPb(void *bus, int *paused) {
    (void)bus;
    if (paused) *paused = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bus::getMute const */
EXPORT int _ZNK4FMOD6Studio3Bus7getMuteEPb(void *bus, int *mute) {
    (void)bus;
    if (mute) *mute = 0;
    return FMOD_OK;
}

/* FMOD::Studio::VCA::isValid const */
EXPORT int _ZNK4FMOD6Studio3VCA7isValidEv(void *vca) {
    return vca ? 1 : 0;
}
/* FMOD::Studio::VCA::getID const */
EXPORT int _ZNK4FMOD6Studio3VCA5getIDEP9FMOD_GUID(void *vca, void *guid) {
    (void)vca; (void)guid;
    return FMOD_OK;
}
/* FMOD::Studio::VCA::getPath const */
EXPORT int _ZNK4FMOD6Studio3VCA7getPathEPciPi(void *vca, char *path, int size, int *ret_size) {
    (void)vca; (void)path; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}
/* FMOD::Studio::VCA::getVolume const */
EXPORT int _ZNK4FMOD6Studio3VCA9getVolumeEPfS2_(void *vca, float *volume, float *finalvol) {
    (void)vca;
    if (volume) *volume = 1.0f;
    if (finalvol) *finalvol = 1.0f;
    return FMOD_OK;
}

/* FMOD::Studio::Bank::isValid const */
EXPORT int _ZNK4FMOD6Studio4Bank7isValidEv(void *bank) {
    return bank ? 1 : 0;
}
/* FMOD::Studio::Bank::getID const */
EXPORT int _ZNK4FMOD6Studio4Bank5getIDEP9FMOD_GUID(void *bank, void *guid) {
    (void)bank; (void)guid;
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getPath const */
EXPORT int _ZNK4FMOD6Studio4Bank7getPathEPciPi(void *bank, char *path, int size, int *ret_size) {
    (void)bank; (void)path; (void)size;
    if (ret_size) *ret_size = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getLoadingState const */
EXPORT int _ZNK4FMOD6Studio4Bank15getLoadingStateEP25FMOD_STUDIO_LOADING_STATE(void *bank, int *state) {
    (void)bank;
    if (state) *state = 2; /* FMOD_STUDIO_LOADING_STATE_LOADED */
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getEventCount const */
EXPORT int _ZNK4FMOD6Studio4Bank13getEventCountEPi(void *bank, int *count) {
    (void)bank;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getEventList const */
EXPORT int _ZNK4FMOD6Studio4Bank12getEventListEPPNS0_16EventDescriptionEiPi(void *bank, void **events, int capacity, int *count) {
    (void)bank; (void)events; (void)capacity;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getBusCount const */
EXPORT int _ZNK4FMOD6Studio4Bank11getBusCountEPi(void *bank, int *count) {
    (void)bank;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getBusList const */
EXPORT int _ZNK4FMOD6Studio4Bank10getBusListEPPNS0_3BusEiPi(void *bank, void **buses, int capacity, int *count) {
    (void)bank; (void)buses; (void)capacity;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getVCACount const */
EXPORT int _ZNK4FMOD6Studio4Bank11getVCACountEPi(void *bank, int *count) {
    (void)bank;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getVCAList const */
EXPORT int _ZNK4FMOD6Studio4Bank10getVCAListEPPNS0_3VCAEiPi(void *bank, void **vcas, int capacity, int *count) {
    (void)bank; (void)vcas; (void)capacity;
    if (count) *count = 0;
    return FMOD_OK;
}
/* FMOD::Studio::Bank::getStringCount const */
EXPORT int _ZNK4FMOD6Studio4Bank14getStringCountEPi(void *bank, int *count) {
    (void)bank;
    if (count) *count = 0;
    return FMOD_OK;
}
