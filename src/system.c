#include "dsa_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static DSA_SystemData *dsa_get_sys(DSA_System *sys) {
    return (DSA_SystemData *)sys->opaque;
}

int DSA_System_Create(DSA_System **system, unsigned int version) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    if (version < 0x00020300) return DSA_ERR_VERSION;

    /* Allocate system data via arena */
    const char *name = "system";
    DSA_SystemData *sys = (DSA_SystemData *)dsa_default_alloc(
        sizeof(DSA_SystemData), sizeof(void *), name
    );
    if (!sys) return DSA_ERR_INTERNAL;
    memset(sys, 0, sizeof(*sys));

    sys->version = version;
    sys->samplerate = DSA_DEFAULT_SAMPLERATE;
    sys->buffer_length = DSA_DEFAULT_BUFFERLENGTH;
    sys->num_buffers = DSA_DEFAULT_NUMBUFFERS;
    sys->doppler_scale = 1.0f;
    sys->distance_factor = 1.0f;
    sys->rolloff_scale = 1.0f;
    sys->num_listeners = 1;

    /* Create arenas for each object type */
    sys->sys_arena = dsa_arena_create("system", DSA_TID_SYSTEM, 256);
    sys->channel_arena = dsa_arena_create("channel", DSA_TID_CHANNEL, 1024);
    sys->sound_arena = dsa_arena_create("sound", DSA_TID_SOUND, 1024);
    sys->channelgroup_arena = dsa_arena_create("channelgroup", DSA_TID_CHANNELGROUP, 256);
    sys->dsp_arena = dsa_arena_create("dsp", DSA_TID_DSP, 256);
    sys->dspconn_arena = dsa_arena_create("dspconn", DSA_TID_DSPCONN, 256);
    sys->soundgroup_arena = dsa_arena_create("soundgroup", DSA_TID_SOUNDGROUP, 64);
    sys->geom_arena = dsa_arena_create("geometry", DSA_TID_GEOMETRY, 64);

    if (!sys->sys_arena || !sys->channel_arena || !sys->sound_arena ||
        !sys->channelgroup_arena || !sys->dsp_arena || !sys->dspconn_arena ||
        !sys->soundgroup_arena || !sys->geom_arena) {
        DSA_System_Release((DSA_System *)&sys);
        return DSA_ERR_INTERNAL;
    }

    /* Allocate system-level objects from sys_arena */
    sys->sounds = (DSA_SoundData **)dsa_arena_alloc(sys->sys_arena,
        sizeof(DSA_SoundData *) * DSA_MAX_SOUNDS, sizeof(void *), __LINE__);
    if (!sys->sounds) { DSA_System_Release((DSA_System *)&sys); return DSA_ERR_INTERNAL; }
    sys->sound_cap = DSA_MAX_SOUNDS;

    /* Create master channel group */
    DSA_ChannelGroupNode *master = (DSA_ChannelGroupNode *)dsa_arena_alloc(
        sys->channelgroup_arena, sizeof(DSA_ChannelGroupNode), sizeof(void *), __LINE__);
    if (!master) { DSA_System_Release((DSA_System *)&sys); return DSA_ERR_INTERNAL; }
    memset(master, 0, sizeof(*master));
    master->id = 1;
    master->volume = 1.0f;
    master->pitch = 1.0f;
    master->refcount = 1;
    sys->master_group = master;

    /* Initialize channel SoA */
    int ch_cap = version >= 0x00020300 ? DSA_MAX_CHANNELS : 32;
    DSA_ChannelSoA *ch = &sys->channels;
    ch->capacity = ch_cap;
    ch->arena = sys->channel_arena;

    ch->volume    = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->pitch     = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->pan       = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->frequency = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->sound_id  = (uint32_t *)dsa_arena_alloc(sys->channel_arena, sizeof(uint32_t) * ch_cap, 4, __LINE__);
    ch->state     = (uint8_t *)dsa_arena_alloc(sys->channel_arena, sizeof(uint8_t) * ch_cap, 1, __LINE__);
    ch->priority  = (uint8_t *)dsa_arena_alloc(sys->channel_arena, sizeof(uint8_t) * ch_cap, 1, __LINE__);
    ch->pos_x     = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->pos_y     = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->pos_z     = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->vel_x     = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->vel_y     = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->vel_z     = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 16, __LINE__);
    ch->min_distance = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 4, __LINE__);
    ch->max_distance = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 4, __LINE__);
    ch->read_pos      = (uint32_t *)dsa_arena_alloc(sys->channel_arena, sizeof(uint32_t) * ch_cap, 4, __LINE__);
    ch->read_pos_frac = (float *)dsa_arena_alloc(sys->channel_arena, sizeof(float) * ch_cap, 4, __LINE__);
    ch->handles   = (DSA_ChannelHandle *)dsa_arena_alloc(sys->channel_arena, sizeof(DSA_ChannelHandle) * ch_cap, sizeof(void *), __LINE__);
    for (int i = 0; i < ch_cap; i++) {
        ch->handles[i].sys = sys;
        ch->handles[i].slot = i;
    }

    /* Initialize all channels to free */
    memset(ch->state, DSA_CHANNELSTATE_FREE, ch_cap);

    /* Initial hash for audit */
    sys->tick = 0;

    DSA_System *result = (DSA_System *)dsa_default_alloc(sizeof(DSA_System), sizeof(void *), name);
    if (!result) { DSA_System_Release((DSA_System *)&sys); return DSA_ERR_INTERNAL; }
    result->opaque = sys;
    *system = result;
    return DSA_OK;
}

int DSA_System_Release(DSA_System *system) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;

    if (sys->output_running) {
        DSA_System_Close(system);
    }

    dsa_arena_destroy(sys->sys_arena);
    dsa_arena_destroy(sys->channel_arena);
    dsa_arena_destroy(sys->sound_arena);
    dsa_arena_destroy(sys->channelgroup_arena);
    dsa_arena_destroy(sys->dsp_arena);
    dsa_arena_destroy(sys->dspconn_arena);
    dsa_arena_destroy(sys->soundgroup_arena);
    dsa_arena_destroy(sys->geom_arena);

    dsa_default_free(sys, sizeof(DSA_SystemData), "system");
    dsa_default_free(system, sizeof(DSA_System), "system_opaque");
    return DSA_OK;
}

int DSA_System_Init(DSA_System *system, int max_channels, unsigned int flags, void *extradriverdata) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    if (sys->initialized) return DSA_ERR_INITIALIZED;
    (void)extradriverdata;

    sys->max_channels = max_channels > 0 ? max_channels : 32;
    if (sys->max_channels > DSA_MAX_CHANNELS)
        sys->max_channels = DSA_MAX_CHANNELS;
    sys->flags = flags;
    sys->initialized = 1;

    /* Initialize audio output (ALSA) */
    sys->output_handle = NULL;

    DSA_OutputBackend *out = (DSA_OutputBackend *)calloc(1, sizeof(DSA_OutputBackend));
    if (!out) return DSA_ERR_INTERNAL;

    if (dsa_output_init(sys, out) == 0) {
        sys->output_handle = out;
        dsa_output_start(sys, out);
        dsa_log( "[dsa] Audio output started (ALSA)\n");
    } else {
        free(out);
        dsa_log( "[dsa] No audio output (silent mode)\n");
    }

    return DSA_OK;
}

int DSA_System_Close(DSA_System *system) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    if (!sys->initialized) return DSA_OK;

    /* TODO: Close audio output device */
    if (sys->output_handle) {
        dsa_output_stop((DSA_OutputBackend *)sys->output_handle);
        free(sys->output_handle);
        sys->output_handle = NULL;
    }
    sys->output_running = 0;
    sys->initialized = 0;
    return DSA_OK;
}

int DSA_System_Update(DSA_System *system) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;

    sys->tick++;

    /* Reset arenas at tick boundary (bulk-free channels, sounds, etc.) */
    /* Only reset if the arena epoch matches the tick */
    if (sys->tick % 10 == 0) {
        /* Periodic audit — could log arena hashes for provenance */
        uint64_t ch_hash = dsa_arena_hash(sys->channel_arena);
        (void)ch_hash;
    }

    return DSA_OK;
}

int DSA_System_SetDSPBufferSize(DSA_System *system, unsigned int bufferlength, int numbuffers) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;

    sys->buffer_length = bufferlength;
    sys->num_buffers = numbuffers;
    return DSA_OK;
}

int DSA_System_GetSoftwareFormat(DSA_System *system, int *samplerate, int *speakermode, int *numrawspeakers) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    if (samplerate) *samplerate = sys->samplerate;
    if (speakermode) *speakermode = sys->speakermode;
    if (numrawspeakers) *numrawspeakers = sys->num_speakers;
    return DSA_OK;
}

int DSA_System_SetSoftwareFormat(DSA_System *system, int samplerate, int speakermode, int numrawspeakers) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    if (samplerate > 0) sys->samplerate = samplerate;
    if (speakermode >= 0) sys->speakermode = speakermode;
    if (numrawspeakers > 0) sys->num_speakers = numrawspeakers;
    return DSA_OK;
}

int DSA_System_Set3DSettings(DSA_System *system, float dopplerscale, float distancefactor, float rolloffscale) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    sys->doppler_scale = dopplerscale;
    sys->distance_factor = distancefactor;
    sys->rolloff_scale = rolloffscale;
    return DSA_OK;
}

int DSA_System_Get3DListenerAttributes(DSA_System *system, int listener, DSA_3DAttributes *attrs) {
    if (!system || !attrs) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    if (listener < 0 || listener >= sys->num_listeners)
        return DSA_ERR_INVALID_HANDLE;
    *attrs = sys->listener[listener];
    return DSA_OK;
}

int DSA_System_Set3DListenerAttributes(DSA_System *system, int listener, const DSA_3DAttributes *attrs) {
    if (!system || !attrs) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    if (listener < 0 || listener >= 4)
        return DSA_ERR_INVALID_HANDLE;
    sys->listener[listener] = *attrs;
    if (listener + 1 > sys->num_listeners)
        sys->num_listeners = listener + 1;
    return DSA_OK;
}

int DSA_System_SetMemorySystem(DSA_System *system, const DSA_MemoryHooks *hooks) {
    if (!system || !hooks) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    sys->mem_hooks = *hooks;
    return DSA_OK;
}

int DSA_System_GetAdvancedSettings(DSA_System *system, DSA_AdvancedSettings *settings) {
    if (!system || !settings) return DSA_ERR_INVALID_HANDLE;
    (void)settings;
    return DSA_ERR_NOTIMPLEMENTED;
}

int DSA_System_SetAdvancedSettings(DSA_System *system, const DSA_AdvancedSettings *settings) {
    if (!system || !settings) return DSA_ERR_INVALID_HANDLE;
    (void)settings;
    return DSA_ERR_NOTIMPLEMENTED;
}

int DSA_System_GetVersion(DSA_System *system, unsigned int *version) {
    if (!system || !version) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    *version = sys->version;
    return DSA_OK;
}

int DSA_System_GetNumDrivers(DSA_System *system, int *numdrivers, int *which) {
    if (!system || !numdrivers) return DSA_ERR_INVALID_HANDLE;
    (void)system;
    /* Stub: report 1 driver (default device) */
    *numdrivers = 1;
    if (which) *which = 0;
    return DSA_OK;
}

int DSA_System_GetDriverInfo(DSA_System *system, int id, char *name, int namelen,
                             void *guid, int *systemrate, int *speakermode,
                             int *speakermodechannels) {
    if (!system) return DSA_ERR_INVALID_HANDLE;
    if (id != 0) return DSA_ERR_INVALID_HANDLE;
    if (name && namelen > 0) {
        snprintf(name, namelen, "DSA Default Audio Driver");
    }
    if (guid) memset(guid, 0, 16); /* null GUID */
    if (systemrate) *systemrate = 44100;
    if (speakermode) *speakermode = 0; /* DSA_SPEAKERMODE_STEREO */
    if (speakermodechannels) *speakermodechannels = 2;
    return DSA_OK;
}

/* Create a sound — real audio data or synthetic sine wave */
int DSA_System_CreateSound(DSA_System *system, const char *name, int mode,
                           const DSA_CreateSoundExInfo *exinfo, DSA_Sound **sound) {
    if (!system || !name || !sound) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;

    DSA_SoundData *sd = (DSA_SoundData *)dsa_arena_alloc(
        sys->sound_arena, sizeof(DSA_SoundData), sizeof(void *), __LINE__);
    if (!sd) return DSA_ERR_INTERNAL;
    memset(sd, 0, sizeof(*sd));

    sd->id = sys->sound_count + 1;
    sd->mode = mode;

    unsigned int num_samples = 0;

    dsa_log( "[dsa] CreateSound: name=%p mode=0x%x have_exinfo=%d exinfo_len=%u\n",
            (const void*)name, mode, exinfo ? 1 : 0, exinfo ? exinfo->length : 0);

    /* If OPENMEMORY is set, name is a pointer to raw audio data */
    if ((mode & DSA_OPENMEMORY) && exinfo && exinfo->length > 0) {
        const char *mem = name;  /* name = pointer to memory data */
        unsigned int mem_len = exinfo->length;
        unsigned int fileoff = exinfo->fileoffset;

        dsa_log( "[dsa] CreateSound OPENMEMORY: name=%p mode=0x%x len=%u off=%u\n",
                (const void*)name, mode, mem_len, fileoff);

        /* Store raw data */
        unsigned int store_len = mem_len - fileoff;
        if (fileoff < mem_len) {
            sd->data = dsa_arena_alloc(sys->sound_arena, store_len, 16, __LINE__);
            if (sd->data && mem) {
                memcpy(sd->data, mem + fileoff, store_len);
            }
            sd->data_size = store_len;
            sd->length = store_len / 2; /* approx PCM samples for 16-bit */
            sd->samplerate = 44100;
            sd->channels = 2;
            sd->bits_per_sample = 16;
            num_samples = (unsigned int)sd->length;

            /* Quick header sniff */
            if (store_len >= 4) {
                const unsigned char *h = (const unsigned char *)sd->data;
                if (h[0] == 'O' && h[1] == 'g' && h[2] == 'g')
                    dsa_log( "[dsa]   -> OGG container\n");
                else if (h[0] == 0xFF && (h[1] & 0xE0) == 0xE0)
                    dsa_log( "[dsa]   -> MP3 sync word\n");
                else if (h[0] == 'R' && h[1] == 'I' && h[2] == 'F' && h[3] == 'F')
                    dsa_log( "[dsa]   -> WAV/RIFF\n");
                else
                    dsa_log( "[dsa]   -> raw data (0x%02x %02x %02x %02x)\n",
                            h[0], h[1], h[2], h[3]);
            }
        }

        /* Decode audio data */
        if (store_len > 0) {
            unsigned int decoded_samples = 0;
            int decoded_channels = 0;
            int decoded_samplerate = 0;
            short *decoded_pcm = dsa_decode_audio(
                (const unsigned char *)sd->data, store_len,
                &decoded_samples, &decoded_channels, &decoded_samplerate);
            if (decoded_pcm) {
                /* Replace raw data with decoded PCM */
                size_t pcm_bytes = decoded_samples * sizeof(short);
                void *pcm_copy = dsa_arena_alloc(sys->sound_arena, pcm_bytes, 16, __LINE__);
                if (pcm_copy) {
                    memcpy(pcm_copy, decoded_pcm, pcm_bytes);
                    free(decoded_pcm);
                    sd->data = pcm_copy;
                    sd->data_size = pcm_bytes;
                    sd->length = decoded_samples;
                    sd->samplerate = decoded_samplerate;
                    sd->channels = decoded_channels;
                    sd->bits_per_sample = 16;
                    num_samples = decoded_samples;
                } else {
                    free(decoded_pcm);
                }
            } else {
                dsa_log( "[dsa]   -> decode failed, using raw\n");
            }
        }
    } else {
        /* Store a display name (not a filename — just for debugging) */
        size_t nlen = strlen(name) + 1;
        if (nlen > 256) nlen = 256;
        sd->name = (char *)dsa_arena_alloc(sys->sound_arena, nlen, 1, __LINE__);
        if (sd->name) memcpy(sd->name, name, nlen);

        /* Generate silent PCM data */
        sd->samplerate = sys->samplerate;
        sd->channels = 1;
        sd->bits_per_sample = 16;
        num_samples = sys->samplerate;
        sd->length = num_samples;
        sd->data_size = num_samples * sizeof(short);
        sd->data = dsa_arena_alloc(sys->sound_arena, sd->data_size, 16, __LINE__);
        if (sd->data) {
            memset(sd->data, 0, sd->data_size);
        }
    }

    /* Add to sound table */
    if (sys->sound_count < sys->sound_cap) {
        sys->sounds[sys->sound_count++] = sd;
    }

    /* Track as last created sound for Studio bridge */
    sys->last_sound = sd;

    *sound = (DSA_Sound *)sd;
    return DSA_OK;
}

int DSA_System_GetLastSound(DSA_System *system, DSA_Sound **sound) {
    if (!system || !sound) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    *sound = (DSA_Sound *)sys->last_sound;
    return *sound ? DSA_OK : DSA_ERR_INVALID_HANDLE;
}

int DSA_Sound_Release(DSA_Sound *sound) {
    if (!sound) return DSA_ERR_INVALID_HANDLE;
    /* Sounds are arena-managed — no individual free needed */
    return DSA_OK;
}

int DSA_Sound_GetMode(DSA_Sound *sound, int *mode) {
    if (!sound || !mode) return DSA_ERR_INVALID_HANDLE;
    DSA_SoundData *sd = (DSA_SoundData *)sound;
    *mode = sd->mode;
    return DSA_OK;
}

int DSA_Sound_SetMode(DSA_Sound *sound, int mode) {
    if (!sound) return DSA_ERR_INVALID_HANDLE;
    DSA_SoundData *sd = (DSA_SoundData *)sound;
    sd->mode = mode;
    return DSA_OK;
}

int DSA_Sound_GetLength(DSA_Sound *sound, unsigned int *length, int lengthunit) {
    if (!sound || !length) return DSA_ERR_INVALID_HANDLE;
    DSA_SoundData *sd = (DSA_SoundData *)sound;
    switch (lengthunit) {
    case DSA_TIMEUNIT_PCMSAMPLES:
        *length = (unsigned int)sd->length;
        break;
    case DSA_TIMEUNIT_PCMBYTES:
        *length = (unsigned int)sd->data_size;
        break;
    case DSA_TIMEUNIT_MS:
        *length = (unsigned int)((uint64_t)sd->length * 1000 / sd->samplerate);
        break;
    default:
        *length = (unsigned int)sd->length;
        break;
    }
    return DSA_OK;
}

int DSA_Sound_GetInfo(DSA_Sound *sound, DSA_SoundInfo *info) {
    if (!sound || !info) return DSA_ERR_INVALID_HANDLE;
    DSA_SoundData *sd = (DSA_SoundData *)sound;
    info->length = (unsigned int)sd->length;
    info->channels = sd->channels;
    info->bits_per_sample = sd->bits_per_sample;
    info->samplerate = sd->samplerate;
    info->mode = sd->mode;
    return DSA_OK;
}

int DSA_System_PlaySound(DSA_System *system, DSA_Sound *sound,
                         DSA_ChannelGroup *group, int paused, DSA_Channel **channel) {
    if (!system || !sound || !channel) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    DSA_SoundData *sd = (DSA_SoundData *)sound;
    if (!sd) return DSA_ERR_INVALID_HANDLE;
    (void)group;

    /* Find a free (or stopped) channel slot */
    DSA_ChannelSoA *ch = &sys->channels;
    int slot = -1;
    for (int i = 0; i < ch->capacity; i++) {
        if (ch->state[i] == DSA_CHANNELSTATE_FREE || ch->state[i] == DSA_CHANNELSTATE_STOPPED) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return DSA_ERR_CHANNEL_ALLOC;

    /* Determine which channel group this channel goes to */
    DSA_ChannelGroupNode *cg = sys->master_group;
    if (group) cg = (DSA_ChannelGroupNode *)group;

    /* Initialize channel state in SoA */
    ch->state[slot]     = paused ? DSA_CHANNELSTATE_PAUSED : DSA_CHANNELSTATE_PLAYING;
    ch->volume[slot]    = 1.0f;
    ch->pitch[slot]     = 1.0f;
    ch->pan[slot]       = 0.0f;
    ch->frequency[slot] = (float)sd->samplerate;
    ch->sound_id[slot]  = sd->id;
    ch->priority[slot]  = 0;
    ch->pos_x[slot]     = 0;
    ch->pos_y[slot]     = 0;
    ch->pos_z[slot]     = 0;
    ch->vel_x[slot]     = 0;
    ch->vel_y[slot]     = 0;
    ch->vel_z[slot]     = 0;
    ch->min_distance[slot] = 1.0f;
    ch->max_distance[slot] = 10000.0f;
    ch->count++;

    /* Track in channel group */
    int cg_idx = cg->num_channels++;
    /* Reallocate channel_ids list as needed (simple: extend from arena) */
    if (cg->channel_ids == NULL) {
        cg->channel_ids = (uint32_t *)dsa_arena_alloc(
            sys->channelgroup_arena, sizeof(uint32_t) * ch->capacity, 4, __LINE__);
    }
    if (cg->channel_ids && cg_idx < ch->capacity) {
        cg->channel_ids[cg_idx] = slot;
    }

    /* Return channel handle */
    *channel = (DSA_Channel *)&ch->handles[slot];
    return DSA_OK;
}

int DSA_Channel_SetVolume(DSA_Channel *channel, float volume) {
    if (!channel) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    h->sys->channels.volume[h->slot] = volume;
    return DSA_OK;
}

int DSA_Channel_SetPitch(DSA_Channel *channel, float pitch) {
    if (!channel) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    h->sys->channels.pitch[h->slot] = pitch;
    return DSA_OK;
}

int DSA_Channel_SetPan(DSA_Channel *channel, float pan) {
    if (!channel) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    h->sys->channels.pan[h->slot] = pan;
    return DSA_OK;
}

int DSA_Channel_IsPlaying(DSA_Channel *channel, int *playing) {
    if (!channel || !playing) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    *playing = (h->sys->channels.state[h->slot] == DSA_CHANNELSTATE_PLAYING) ? 1 : 0;
    return DSA_OK;
}

int DSA_Channel_Stop(DSA_Channel *channel) {
    if (!channel) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    h->sys->channels.state[h->slot] = DSA_CHANNELSTATE_STOPPED;
    return DSA_OK;
}

int DSA_Channel_Set3DAttributes(DSA_Channel *channel, const DSA_3DAttributes *attrs) {
    if (!channel || !attrs) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelSoA *ch = &h->sys->channels;
    ch->pos_x[h->slot] = attrs->position[0];
    ch->pos_y[h->slot] = attrs->position[1];
    ch->pos_z[h->slot] = attrs->position[2];
    ch->vel_x[h->slot] = attrs->velocity[0];
    ch->vel_y[h->slot] = attrs->velocity[1];
    ch->vel_z[h->slot] = attrs->velocity[2];
    return DSA_OK;
}

int DSA_Channel_Set3DMinMaxDistance(DSA_Channel *channel, float mindistance, float maxdistance) {
    if (!channel) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelSoA *ch = &h->sys->channels;
    ch->min_distance[h->slot] = mindistance > 0.0f ? mindistance : 1.0f;
    ch->max_distance[h->slot] = maxdistance > 0.0f ? maxdistance : 10000.0f;
    return DSA_OK;
}

int DSA_Channel_GetFrequency(DSA_Channel *channel, float *frequency) {
    if (!channel || !frequency) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    *frequency = h->sys->channels.frequency[h->slot];
    return DSA_OK;
}

int DSA_Channel_SetFrequency(DSA_Channel *channel, float frequency) {
    if (!channel) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelHandle *h = (DSA_ChannelHandle *)channel;
    if (!h->sys) return DSA_ERR_INVALID_HANDLE;
    if (h->slot < 0 || h->slot >= h->sys->channels.capacity) return DSA_ERR_INVALID_HANDLE;
    h->sys->channels.frequency[h->slot] = frequency;
    return DSA_OK;
}

int DSA_Channel_GetPosition(DSA_Channel *channel, unsigned int *position, int timeunit) {
    if (!channel || !position) return DSA_ERR_INVALID_HANDLE;
    (void)timeunit;
    *position = 0;
    return DSA_OK;
}

int DSA_Channel_SetPosition(DSA_Channel *channel, unsigned int position, int timeunit) {
    if (!channel) return DSA_ERR_INVALID_HANDLE;
    (void)position; (void)timeunit;
    return DSA_OK;
}

int DSA_Channel_GetCurrentSound(DSA_Channel *channel, DSA_Sound **sound) {
    if (!channel || !sound) return DSA_ERR_INVALID_HANDLE;
    *sound = NULL;
    return DSA_OK;
}

int DSA_Channel_GetChannelGroup(DSA_Channel *channel, DSA_ChannelGroup **group) {
    if (!channel || !group) return DSA_ERR_INVALID_HANDLE;
    *group = NULL;
    return DSA_OK;
}

int DSA_Channel_AddDSP(DSA_Channel *channel, DSA_DSP *dsp, DSA_DSPConnection **connection) {
    if (!channel || !dsp) return DSA_ERR_INVALID_HANDLE;
    (void)connection;
    return DSA_OK;
}

/* === ChannelGroup === */

int DSA_System_GetMasterChannelGroup(DSA_System *system, DSA_ChannelGroup **group) {
    if (!system || !group) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    *group = (DSA_ChannelGroup *)sys->master_group;
    return DSA_OK;
}

int DSA_ChannelGroup_Release(DSA_ChannelGroup *group) {
    if (!group) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelGroupNode *cg = (DSA_ChannelGroupNode *)group;
    if (cg->refcount > 1) { cg->refcount--; return DSA_OK; }
    if (cg->refcount > 0) cg->refcount--;
    return DSA_OK;
}

int DSA_ChannelGroup_Stop(DSA_ChannelGroup *group) {
    if (!group) return DSA_ERR_INVALID_HANDLE;
    /* Arena-managed: channels freed at tick boundary */
    return DSA_OK;
}

int DSA_ChannelGroup_AddDSP(DSA_ChannelGroup *group, DSA_DSP *dsp, DSA_DSPConnection **connection) {
    if (!group || !dsp) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelGroupNode *cg = (DSA_ChannelGroupNode *)group;
    DSA_DSPNode *node = (DSA_DSPNode *)dsp;
    (void)connection;

    /* Add to DSP chain */
    if (cg->dsp_count == 0) {
        cg->dsp_chain = (DSA_DSPNode **)malloc(sizeof(DSA_DSPNode *) * 16);
    }
    if (cg->dsp_count < 64) {
        cg->dsp_chain[cg->dsp_count++] = node;
    }

    if (connection) {
        /* Create a DSP connection object */
        /* For now, return NULL — connection creation via arena TBD */
        *connection = NULL;
    }
    return DSA_OK;
}

int DSA_ChannelGroup_GetNumChannels(DSA_ChannelGroup *group, int *num) {
    if (!group || !num) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelGroupNode *cg = (DSA_ChannelGroupNode *)group;
    *num = cg->num_channels;
    return DSA_OK;
}

int DSA_ChannelGroup_GetChannel(DSA_ChannelGroup *group, int index, DSA_Channel **channel) {
    if (!group || !channel) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelGroupNode *cg = (DSA_ChannelGroupNode *)group;
    if (index < 0 || index >= cg->num_channels) return DSA_ERR_INVALID_HANDLE;
    (void)channel;
    return DSA_ERR_NOTIMPLEMENTED;
}

int DSA_ChannelGroup_SetVolume(DSA_ChannelGroup *group, float volume) {
    if (!group) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelGroupNode *cg = (DSA_ChannelGroupNode *)group;
    cg->volume = volume;
    return DSA_OK;
}

int DSA_ChannelGroup_SetPitch(DSA_ChannelGroup *group, float pitch) {
    if (!group) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelGroupNode *cg = (DSA_ChannelGroupNode *)group;
    cg->pitch = pitch;
    return DSA_OK;
}

int DSA_ChannelGroup_GetVolume(DSA_ChannelGroup *group, float *volume) {
    if (!group || !volume) return DSA_ERR_INVALID_HANDLE;
    DSA_ChannelGroupNode *cg = (DSA_ChannelGroupNode *)group;
    *volume = cg->volume;
    return DSA_OK;
}

/* === DSP === */

int DSA_System_CreateDSPByType(DSA_System *system, int type, DSA_DSP **dsp) {
    if (!system || !dsp) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;

    DSA_DSPNode *node = (DSA_DSPNode *)dsa_arena_alloc(
        sys->dsp_arena, sizeof(DSA_DSPNode), sizeof(void *), __LINE__);
    if (!node) return DSA_ERR_INTERNAL;
    memset(node, 0, sizeof(*node));

    node->id = sys->dsp_count + 1;
    node->type = type;
    node->active = 1;

    /* Set default parameters based on type */
    switch (type) {
    case DSA_DSP_TYPE_LOWPASS:
        snprintf(node->name, sizeof(node->name), "Lowpass");
        node->params[0] = 500.0f;  /* cutoff frequency */
        node->params[1] = 1.0f;    /* resonance */
        break;
    case DSA_DSP_TYPE_HIGHPASS:
        snprintf(node->name, sizeof(node->name), "Highpass");
        node->params[0] = 500.0f;
        node->params[1] = 1.0f;
        break;
    case DSA_DSP_TYPE_OSCILLATOR:
        snprintf(node->name, sizeof(node->name), "Oscillator");
        node->params[0] = 220.0f; /* frequency */
        node->params[1] = 0.5f;   /* amplitude */
        break;
    case DSA_DSP_TYPE_ECHO:
        snprintf(node->name, sizeof(node->name), "Echo");
        node->params[0] = 200.0f; /* delay (ms) */
        node->params[1] = 0.5f;   /* decay */
        break;
    case DSA_DSP_TYPE_REVERB:
    case DSA_DSP_TYPE_SFXREVERB:
        snprintf(node->name, sizeof(node->name), "Reverb");
        node->params[0] = 0.3f; /* decay time */
        node->params[1] = 0.5f; /* wet level */
        break;
    default:
        snprintf(node->name, sizeof(node->name), "DSP type %d", type);
        break;
    }

    node->num_inputs = 1;
    node->num_outputs = 1;

    if (sys->dsp_count < 256)
        sys->dsps[sys->dsp_count++] = node;

    dsa_dsp_bind_process(node);

    *dsp = (DSA_DSP *)node;
    return DSA_OK;
}

int DSA_DSP_Release(DSA_DSP *dsp) {
    if (!dsp) return DSA_ERR_INVALID_HANDLE;
    dsa_dsp_free_state((DSA_DSPNode *)dsp);
    return DSA_OK;
}

int DSA_DSP_GetInfo(DSA_DSP *dsp, char *name, unsigned int *version,
                    int *channels, int *configwidth, int *configheight) {
    if (!dsp) return DSA_ERR_INVALID_HANDLE;
    DSA_DSPNode *node = (DSA_DSPNode *)dsp;
    if (name) {
        size_t nlen = strlen(node->name) + 1;
        if (nlen > 64) nlen = 64;
        memcpy(name, node->name, nlen);
    }
    if (version) *version = 1;
    if (channels) *channels = 2;
    if (configwidth) *configwidth = 0;
    if (configheight) *configheight = 0;
    return DSA_OK;
}

int DSA_DSP_AddDSP(DSA_DSP *dsp, DSA_DSP *connection, DSA_DSPConnection **conn) {
    if (!dsp || !connection) return DSA_ERR_INVALID_HANDLE;
    (void)conn;
    return DSA_OK;
}

int DSA_DSP_Remove(DSA_DSP *dsp) {
    if (!dsp) return DSA_ERR_INVALID_HANDLE;
    return DSA_OK;
}

int DSA_DSP_GetNumInputs(DSA_DSP *dsp, int *num) {
    if (!dsp || !num) return DSA_ERR_INVALID_HANDLE;
    DSA_DSPNode *node = (DSA_DSPNode *)dsp;
    *num = node->num_inputs;
    return DSA_OK;
}

int DSA_DSP_GetNumOutputs(DSA_DSP *dsp, int *num) {
    if (!dsp || !num) return DSA_ERR_INVALID_HANDLE;
    DSA_DSPNode *node = (DSA_DSPNode *)dsp;
    *num = node->num_outputs;
    return DSA_OK;
}

int DSA_DSP_SetParameterFloat(DSA_DSP *dsp, int index, float value) {
    if (!dsp) return DSA_ERR_INVALID_HANDLE;
    DSA_DSPNode *node = (DSA_DSPNode *)dsp;
    if (index < 0 || index >= 16) return DSA_ERR_INVALID_HANDLE;
    node->params[index] = value;
    return DSA_OK;
}

int DSA_DSP_GetParameterFloat(DSA_DSP *dsp, int index, float *value) {
    if (!dsp || !value) return DSA_ERR_INVALID_HANDLE;
    DSA_DSPNode *node = (DSA_DSPNode *)dsp;
    if (index < 0 || index >= 16) return DSA_ERR_INVALID_HANDLE;
    *value = node->params[index];
    return DSA_OK;
}

int DSA_DSP_SetActive(DSA_DSP *dsp, int active) {
    if (!dsp) return DSA_ERR_INVALID_HANDLE;
    DSA_DSPNode *node = (DSA_DSPNode *)dsp;
    node->active = active;
    return DSA_OK;
}

/* === DSPConnection === */

int DSA_DSPConnection_SetMix(DSA_DSPConnection *conn, float mix) {
    if (!conn) return DSA_ERR_INVALID_HANDLE;
    DSA_DSPConn *c = (DSA_DSPConn *)conn;
    c->mix = mix;
    return DSA_OK;
}

int DSA_DSPConnection_GetMix(DSA_DSPConnection *conn, float *mix) {
    if (!conn || !mix) return DSA_ERR_INVALID_HANDLE;
    DSA_DSPConn *c = (DSA_DSPConn *)conn;
    *mix = c->mix;
    return DSA_OK;
}

/* === SoundGroup === */

int DSA_System_CreateSoundGroup(DSA_System *system, const char *name, DSA_SoundGroup **group) {
    if (!system || !name || !group) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;

    DSA_SoundGroupData *sg = (DSA_SoundGroupData *)dsa_arena_alloc(
        sys->soundgroup_arena, sizeof(DSA_SoundGroupData), sizeof(void *), __LINE__);
    if (!sg) return DSA_ERR_INTERNAL;
    memset(sg, 0, sizeof(*sg));

    sg->id = sys->soundgroup_count + 1;
    sg->volume = 1.0f;

    size_t nlen = strlen(name) + 1;
    sg->name = (char *)dsa_arena_alloc(sys->soundgroup_arena, nlen, 1, __LINE__);
    if (sg->name) memcpy(sg->name, name, nlen);

    if (sys->soundgroup_count < 64)
        sys->soundgroups[sys->soundgroup_count++] = sg;

    *group = (DSA_SoundGroup *)sg;
    return DSA_OK;
}

int DSA_SoundGroup_Release(DSA_SoundGroup *group) {
    if (!group) return DSA_ERR_INVALID_HANDLE;
    return DSA_OK;
}

int DSA_SoundGroup_GetNumPlaying(DSA_SoundGroup *group, int *num) {
    if (!group || !num) return DSA_ERR_INVALID_HANDLE;
    *num = 0;
    return DSA_OK;
}

int DSA_SoundGroup_SetVolume(DSA_SoundGroup *group, float volume) {
    if (!group) return DSA_ERR_INVALID_HANDLE;
    DSA_SoundGroupData *sg = (DSA_SoundGroupData *)group;
    sg->volume = volume;
    return DSA_OK;
}

int DSA_SoundGroup_GetVolume(DSA_SoundGroup *group, float *volume) {
    if (!group || !volume) return DSA_ERR_INVALID_HANDLE;
    DSA_SoundGroupData *sg = (DSA_SoundGroupData *)group;
    *volume = sg->volume;
    return DSA_OK;
}

/* === Geometry === */

int DSA_System_CreateGeometry(DSA_System *system, int maxpolygons, int maxvertices, DSA_Geometry **geometry) {
    if (!system || !geometry) return DSA_ERR_INVALID_HANDLE;
    DSA_SystemData *sys = dsa_get_sys(system);
    if (!sys) return DSA_ERR_INVALID_HANDLE;
    (void)maxpolygons; (void)maxvertices;

    DSA_GeometryData *geom = (DSA_GeometryData *)dsa_arena_alloc(
        sys->geom_arena, sizeof(DSA_GeometryData), sizeof(void *), __LINE__);
    if (!geom) return DSA_ERR_INTERNAL;
    memset(geom, 0, sizeof(*geom));

    geom->id = sys->geometry_count + 1;
    if (sys->geometry_count < 64)
        sys->geometries[sys->geometry_count++] = geom;

    *geometry = (DSA_Geometry *)geom;
    return DSA_OK;
}

int DSA_Geometry_Release(DSA_Geometry *geometry) {
    if (!geometry) return DSA_ERR_INVALID_HANDLE;
    return DSA_OK;
}
