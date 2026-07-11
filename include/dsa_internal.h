#ifndef DSA_INTERNAL_H_
#define DSA_INTERNAL_H_

#define _POSIX_C_SOURCE 200112L

#include "dsa.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

/* Log to a file so the user can check /tmp/dsa.log even from Steam */
static inline void dsa_log(const char *fmt, ...) {
    FILE *f = fopen("/tmp/dsa.log", "a");
    if (!f) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fclose(f);
}

/* Forward declarations */
typedef struct DSA_ChannelGroupNode DSA_ChannelGroupNode;
typedef struct DSA_DSPNode DSA_DSPNode;
typedef struct DSA_SystemData DSA_SystemData;

/* === Arena allocator — DSO pattern ===
 *
 * Cache-line-aligned bump allocator. Each object type gets its own arena.
 * Bulk-free at tick boundary. Chain of blocks with prev_hash for audit.
 */

#define DSA_CACHE_LINE     64
#define DSA_ARENA_BLOCK_SHIFT 16  /* 64KB blocks */
#define DSA_ARENA_BLOCK_SIZE (1 << DSA_ARENA_BLOCK_SHIFT)
#define DSA_PAGE_SIZE      4096

/* Hash audit entry for provenance tracking */
typedef struct {
    uint64_t prev_hash;
    uint64_t obj_hash;
    size_t   size;
    uint32_t type_id;
    uint32_t line;
} DSA_AuditEntry;

/* Arena — a chain of cache-aligned blocks */
typedef struct DSA_ArenaBlock {
    struct DSA_ArenaBlock *next;
    uint64_t  hash;
    size_t    capacity;
    size_t    used;
    uintptr_t data_start;
} DSA_ArenaBlock;

typedef struct {
    DSA_ArenaBlock *head;
    DSA_ArenaBlock *tail;
    size_t          block_capacity;
    const char     *type_name;
    uint32_t        type_id;
    uint64_t        epoch;
    uint64_t        total_allocated;
    uint64_t        total_freed;
    DSA_AuditEntry *audit;
    int             audit_count;
    int             audit_cap;
    uint64_t        audit_hash;
} DSA_Arena;

/* Arena API */
DSA_Arena *dsa_arena_create(const char *type_name, uint32_t type_id, int audit_cap);
void       dsa_arena_destroy(DSA_Arena *arena);
void      *dsa_arena_alloc(DSA_Arena *arena, size_t size, size_t alignment, uint32_t line);
void       dsa_arena_reset(DSA_Arena *arena);
uint64_t   dsa_arena_hash(DSA_Arena *arena);

/* Channel handle */
typedef struct {
    DSA_SystemData *sys;
    int slot;
} DSA_ChannelHandle;

/* Channel SoA (Struct of Arrays) */
typedef struct {
    float   *volume;
    float   *pitch;
    float   *pan;
    float   *frequency;
    uint32_t *sound_id;
    uint8_t  *state;
    uint8_t  *priority;
    uint32_t *read_pos;       /* current play position in PCM samples */
    float    *read_pos_frac;  /* fractional part for pitch interpolation */
    float   *pos_x, *pos_y, *pos_z;
    float   *vel_x, *vel_y, *vel_z;
    float   *min_distance, *max_distance;
    int      capacity;
    int      count;
    DSA_Arena *arena;
    DSA_ChannelHandle *handles;
} DSA_ChannelSoA;

/* ChannelGroup */
typedef struct DSA_ChannelGroupNode {
    uint32_t        id;
    char           *name;
    float           volume;
    float           pitch;
    int             num_channels;
    uint32_t       *channel_ids;
    DSA_DSPNode   **dsp_chain;
    int             dsp_count;
    DSA_ChannelGroupNode *parent;
    int             refcount;
} DSA_ChannelGroupNode;

/* Sound */
typedef struct {
    uint32_t id;
    char    *name;
    int      mode;
    int      channels;
    int      bits_per_sample;
    int      samplerate;
    void    *data;
    size_t   data_size;
    size_t   length;
    DSA_SoundGroup *group;
} DSA_SoundData;

/* DSP node */
typedef struct DSA_DSPNode {
    uint32_t  id;
    int       type;
    char      name[64];
    int       active;
    float     params[16];
    int       num_inputs;
    int       num_outputs;
    DSA_DSPNode **inputs;
    DSA_DSPNode **outputs;
    DSA_DSPConnection **connections;
    int       input_cap;
    int       output_cap;
    void (*process)(struct DSA_DSPNode *dsp, float *in, float *out, int frames, int channels);
    void *userdata;
} DSA_DSPNode;

/* DSP connection */
typedef struct {
    uint32_t id;
    DSA_DSPNode *src;
    DSA_DSPNode *dst;
    float mix;
} DSA_DSPConn;

/* Sound group */
typedef struct {
    uint32_t id;
    char    *name;
    float    volume;
    int      num_sounds;
    uint32_t *sound_ids;
} DSA_SoundGroupData;

/* Geometry stub */
typedef struct {
    uint32_t id;
} DSA_GeometryData;

/* System (main singleton) */
union DSA_System {
    void *opaque;
};

struct DSA_SystemData {
    unsigned int version;
    int          initialized;
    int          max_channels;
    unsigned int flags;
    int          samplerate;
    int          buffer_length;
    int          num_buffers;
    int          speakermode;
    int          num_speakers;
    float        doppler_scale;
    float        distance_factor;
    float        rolloff_scale;
    DSA_MemoryHooks mem_hooks;
    DSA_Arena *sys_arena;
    DSA_Arena *channel_arena;
    DSA_Arena *sound_arena;
    DSA_Arena *channelgroup_arena;
    DSA_Arena *dsp_arena;
    DSA_Arena *dspconn_arena;
    DSA_Arena *soundgroup_arena;
    DSA_Arena *geom_arena;
    DSA_ChannelSoA channels;
    DSA_SoundData       **sounds;
    int                    sound_count;
    int                    sound_cap;
    DSA_SoundData         *last_sound;   /* for Studio EventInstance bridge */
    DSA_ChannelGroupNode **channelgroups;
    int                    channelgroup_count;
    DSA_ChannelGroupNode  *master_group;
    DSA_DSPNode           *dsps[256];
    int                    dsp_count;
    DSA_DSPConn           *dsp_connections[256];
    int                    dspconn_count;
    DSA_SoundGroupData    *soundgroups[64];
    int                    soundgroup_count;
    DSA_GeometryData      *geometries[64];
    int                    geometry_count;
    DSA_Reverb *reverbs[4];
    int         reverb_count;
    void       *output_handle;
    int         output_running;
    uint64_t    tick;
    DSA_3DAttributes listener[4];
    int num_listeners;
};

/* === Arena implementation helpers === */

static inline uint64_t dsa_hash_combine(uint64_t h, uint64_t v) {
    return h ^ (v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
}

static inline uint64_t dsa_ptr_hash(const void *p) {
    uintptr_t v = (uintptr_t)p;
    return v * 0x9e3779b97f4a7c15ULL;
}

/* Type IDs for arenas */
#define DSA_TID_SYSTEM         1
#define DSA_TID_CHANNEL        2
#define DSA_TID_SOUND          3
#define DSA_TID_CHANNELGROUP   4
#define DSA_TID_DSP            5
#define DSA_TID_DSPCONN        6
#define DSA_TID_SOUNDGROUP     7
#define DSA_TID_GEOMETRY       8

/* Internal helpers */
static inline int dsa_is_power2(size_t v) {
    return v && !(v & (v - 1));
}

static inline size_t dsa_align_up(size_t v, size_t a) {
    return (v + a - 1) & ~(a - 1);
}

static inline void *dsa_default_alloc(size_t size, size_t alignment, const char *type) {
    (void)type;
    if (alignment <= sizeof(void*))
        return malloc(size);
    void *p = NULL;
    if (posix_memalign(&p, alignment, size) != 0)
        return NULL;
    return p;
}

static inline void dsa_default_free(void *ptr, size_t size, const char *type) {
    (void)size; (void)type;
    free(ptr);
}

/* DSP process binding from mixer.c */
void dsa_dsp_bind_process(DSA_DSPNode *dsp);
void dsa_dsp_free_state(DSA_DSPNode *dsp);

/* Per-DSP runtime state (filter memory, delay lines, LFO phases) */
typedef struct {
    /* biquad state (lowpass/highpass/parameq/itlowpass) per channel */
    float b_x1[8], b_x2[8], b_y1[8], b_y2[8];

    /* echo / flanger single delay line (stereo interleaved) */
    float *delay;
    int    delay_cap;
    int    delay_pos;

    /* reverb: 4 parallel comb + 2 allpass (mono per channel, up to 2ch) */
    float *comb[2][4];
    int    comb_cap[4];
    int    comb_pos[4];
    float *ap[2][2];
    int    ap_cap[2];
    int    ap_pos[2];

    /* LFO phase for flanger/tremolo/oscillator */
    double lfo_phase;

    /* pitch shift ring buffer (mono) */
    float *ps_buf;
    int    ps_cap;
    int    ps_in;
    int    ps_count;   /* monotonic input sample counter */

    /* compressor envelope follower */
    float  comp_env;
} DSA_DSPState;

/* Mixer tick — fills output buffer with mixed audio */
void dsa_mixer_tick(DSA_SystemData *sys, float *output, int frames, int channels);

/* Audio decoder — decodes MP3/WAV/raw to 16-bit PCM (returns malloc'd buffer) */
short *dsa_decode_audio(const unsigned char *data, size_t size,
                        unsigned int *out_samples, int *out_channels,
                        int *out_samplerate);

/* Output backend API — called from mixer thread */
typedef struct {
    void       *handle;      /* ALSA snd_pcm_t */
    int         samplerate;
    int         channels;
    int         buffer_frames;
    int         running;
    void       *mix_buf;     /* pre-allocated mix buffer */
} DSA_OutputBackend;

int  dsa_output_init(DSA_SystemData *sys, DSA_OutputBackend *out);
int  dsa_output_start(DSA_SystemData *sys, DSA_OutputBackend *out);
void dsa_output_stop(DSA_OutputBackend *out);
void dsa_output_mix_and_write(DSA_SystemData *sys, DSA_OutputBackend *out);

#endif /* DSA_INTERNAL_H_ */
