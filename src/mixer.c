/* Tick-based mixer — DSO pattern core.
 *
 * Processes all active channels once per audio tick.
 * SoA layout enables efficient SIMD processing.
 * Arenas reset at tick boundary for bulk-free.
 */

#include "dsa_internal.h"
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Forward declarations for DSP processors */
static void dsp_process_lowpass(DSA_DSPNode *dsp, float *in, float *out,
                                int frames, int channels);
static void dsp_process_itlowpass(DSA_DSPNode *dsp, float *in, float *out,
                                  int frames, int channels);
static void dsp_process_highpass(DSA_DSPNode *dsp, float *in, float *out,
                                 int frames, int channels);
static void dsp_process_parameq(DSA_DSPNode *dsp, float *in, float *out,
                                int frames, int channels);
static void dsp_process_distortion(DSA_DSPNode *dsp, float *in, float *out,
                                   int frames, int channels);
static void dsp_process_flanger(DSA_DSPNode *dsp, float *in, float *out,
                                int frames, int channels);
static void dsp_process_tremolo(DSA_DSPNode *dsp, float *in, float *out,
                                int frames, int channels);
static void dsp_process_echo(DSA_DSPNode *dsp, float *in, float *out,
                             int frames, int channels);
static void dsp_process_reverb(DSA_DSPNode *dsp, float *in, float *out,
                               int frames, int channels);
static void dsp_process_compressor(DSA_DSPNode *dsp, float *in, float *out,
                                   int frames, int channels);
static void dsp_process_normalize(DSA_DSPNode *dsp, float *in, float *out,
                                  int frames, int channels);
static void dsp_process_pitchshift(DSA_DSPNode *dsp, float *in, float *out,
                                   int frames, int channels);
static void dsp_process_oscillator(DSA_DSPNode *dsp, float *in, float *out,
                                   int frames, int channels);
static void dsp_process_passthrough(DSA_DSPNode *dsp, float *in, float *out,
                                    int frames, int channels);

/* Mix a single channel: read from sound PCM, apply volume/pitch/pan, sum to output */
static void mix_channel(DSA_SystemData *sys, int slot,
                        float *out_l, float *out_r, int frames) {
    DSA_ChannelSoA *ch = &sys->channels;
    if (ch->state[slot] != DSA_CHANNELSTATE_PLAYING) return;

    uint32_t sound_id = ch->sound_id[slot];
    if (sound_id == 0) return;

    /* Find sound by ID */
    DSA_SoundData *sd = NULL;
    for (int i = 0; i < sys->sound_count; i++) {
        if (sys->sounds[i]->id == sound_id) {
            sd = sys->sounds[i];
            break;
        }
    }
    if (!sd || !sd->data) {
        ch->state[slot] = DSA_CHANNELSTATE_STOPPED;
        return;
    }

    float vol = ch->volume[slot];
    float pan = ch->pan[slot];       /* -1..1 */

    /* 3D spatialization: distance attenuation + azimuth pan */
    if (sd->mode & DSA_3D) {
        DSA_3DAttributes *L = &sys->listener[0];
        float fx = L->forward[0], fy = L->forward[1], fz = L->forward[2];
        float ux = L->up[0], uy = L->up[1], uz = L->up[2];
        float flen = sqrtf(fx * fx + fy * fy + fz * fz);
        if (flen < 1e-4f) { fx = 0; fy = 0; fz = 1; ux = 0; uy = 1; uz = 0; }
        else { fx /= flen; fy /= flen; fz /= flen; ux /= flen; uy /= flen; uz /= flen; }
        /* right = forward x up */
        float rx = fy * uz - fz * uy;
        float ry = fz * ux - fx * uz;
        float rz = fx * uy - fy * ux;
        float dx = ch->pos_x[slot] - L->position[0];
        float dy = ch->pos_y[slot] - L->position[1];
        float dz = ch->pos_z[slot] - L->position[2];
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        float min = ch->min_distance[slot];
        float max = ch->max_distance[slot];
        float rolloff = sys->rolloff_scale > 0.0f ? sys->rolloff_scale : 1.0f;
        float gain;
        if (dist <= min) gain = 1.0f;
        else if (dist >= max) gain = 0.0f;
        else gain = min / (min + rolloff * (dist - min));   /* inverse rolloff */
        vol *= gain;
        float fdot = dx * fx + dy * fy + dz * fz;
        float rdot = dx * rx + dy * ry + dz * rz;
        float az = (dist > 1e-4f) ? rdot / dist : 0.0f;     /* sin(azimuth) */
        pan = az > 1.0f ? 1.0f : (az < -1.0f ? -1.0f : az);
        (void)fdot;
    }

    short *pcm = (short *)sd->data;
    unsigned int num_samples = (unsigned int)sd->length;
    unsigned int read_pos = ch->read_pos[slot];
    float read_frac = ch->read_pos_frac[slot];
    float pitch_ratio = ch->pitch[slot];

    /* Pitch = variable-rate sample read with linear interpolation (SRC).
     * Higher ratio -> faster playback (higher pitch); <1 -> slower. */
    if (pitch_ratio <= 0.0f) pitch_ratio = 1.0f;

    float pan_l = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
    float pan_r = (pan >= 0.0f) ? 1.0f : (1.0f + pan);

    for (int f = 0; f < frames && read_pos < num_samples; f++) {
        float sample_l = 0, sample_r = 0;
        int idx = (int)read_pos;
        int next_idx = idx + 1;
        float frac = read_frac;

        /* Linear interpolation between samples for pitch */
        if (sd->channels == 1) {
            float s0 = pcm[idx] / 32768.0f;
            float s1 = (next_idx < (int)num_samples) ? pcm[next_idx] / 32768.0f : 0;
            sample_l = sample_r = s0 + (s1 - s0) * frac;
        } else if (sd->channels >= 2) {
            float s0_l = pcm[idx * 2] / 32768.0f;
            float s0_r = pcm[idx * 2 + 1] / 32768.0f;
            float s1_l = (next_idx < (int)num_samples) ? pcm[next_idx * 2] / 32768.0f : 0;
            float s1_r = (next_idx < (int)num_samples) ? pcm[next_idx * 2 + 1] / 32768.0f : 0;
            sample_l = s0_l + (s1_l - s0_l) * frac;
            sample_r = s0_r + (s1_r - s0_r) * frac;
        }

        out_l[f] += sample_l * vol * pan_l;
        out_r[f] += sample_r * vol * pan_r;

        /* Advance read position with pitch */
        float step = pitch_ratio;
        read_frac += step - (int)step;
        read_pos += (int)step;
        if (read_frac >= 1.0f) {
            read_frac -= 1.0f;
            read_pos++;
        }
    }

    ch->read_pos[slot] = read_pos;
    ch->read_pos_frac[slot] = read_frac;

    if (read_pos >= num_samples) {
        ch->state[slot] = DSA_CHANNELSTATE_FREE;
        ch->sound_id[slot] = 0;
        ch->count--;
    }
}

/* Process DSP chain for a channel group */
static void process_dsp_chain(DSA_ChannelGroupNode *group,
                              float *in_l, float *in_r,
                              float *out_l, float *out_r,
                              int frames) {
    /* Apply group volume */
    float gvol = group->volume;
    if (gvol != 1.0f) {
        for (int i = 0; i < frames; i++) {
            in_l[i] *= gvol;
            in_r[i] *= gvol;
        }
    }

    /* Process DSP chain */
    for (int d = 0; d < group->dsp_count; d++) {
        DSA_DSPNode *dsp = group->dsp_chain[d];
        if (!dsp || !dsp->active) continue;

        /* Interleave for DSP processing */
        float interleaved[1024 * 2]; /* max frames * 2ch */
        /* TODO: allocate from stack/arena for larger buffer sizes */
        int total = frames * 2;
        for (int i = 0; i < frames; i++) {
            interleaved[i * 2]     = in_l[i];
            interleaved[i * 2 + 1] = in_r[i];
        }
        float processed[1024 * 2];
        memset(processed, 0, sizeof(float) * total);

        if (dsp->process) {
            dsp->process(dsp, interleaved, processed, frames, 2);
        }

        for (int i = 0; i < frames; i++) {
            out_l[i] += processed[i * 2];
            out_r[i] += processed[i * 2 + 1];
        }
    }
}

/* Main tick: mixes all channels into output buffer */
void dsa_mixer_tick(DSA_SystemData *sys, float *output, int frames, int channels) {
    if (!sys || !sys->initialized || !output || frames <= 0) return;
    if (frames > 4096) frames = 4096; /* safety cap */

    float mix_l[4096], mix_r[4096];
    memset(mix_l, 0, sizeof(float) * frames);
    memset(mix_r, 0, sizeof(float) * frames);

    DSA_ChannelSoA *ch = &sys->channels;

    /* Mix all active channels */
    for (int i = 0; i < ch->capacity; i++) {
        if (ch->state[i] == DSA_CHANNELSTATE_PLAYING) {
            mix_channel(sys, i, mix_l, mix_r, frames);
        }
    }

    /* Process master group DSP chain */
    if (sys->master_group && sys->master_group->dsp_count > 0) {
        float dsp_out_l[4096], dsp_out_r[4096];
        memset(dsp_out_l, 0, sizeof(float) * frames);
        memset(dsp_out_r, 0, sizeof(float) * frames);
        process_dsp_chain(sys->master_group, mix_l, mix_r,
                          dsp_out_l, dsp_out_r, frames);
        memcpy(mix_l, dsp_out_l, sizeof(float) * frames);
        memcpy(mix_r, dsp_out_r, sizeof(float) * frames);
    }

    /* Interleave into output buffer */
    for (int i = 0; i < frames; i++) {
        if (channels >= 1) output[i * channels]     = mix_l[i];
        if (channels >= 2) output[i * channels + 1] = mix_r[i];
    }
}

/* === DSP process implementations ===
 *
 * All effects share a DSA_DSPState attached to dsp->userdata.
 * Samplerate is fixed at 44100 (DSA default) for coefficient math.
 */

#define DSA_DSP_SR 44100.0f

/* ---- RBJ biquad helpers (per-channel direct form II transposed) ---- */
static void dsp_biquad(DSA_DSPNode *dsp, float *in, float *out,
                       int frames, int channels,
                       float b0, float b1, float b2,
                       float a1, float a2) {
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    int chans = channels > 8 ? 8 : channels;
    for (int c = 0; c < chans; c++) {
        float x1 = s->b_x1[c], x2 = s->b_x2[c], y1 = s->b_y1[c], y2 = s->b_y2[c];
        for (int f = 0; f < frames; f++) {
            float x = in[f * channels + c];
            float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x;
            y2 = y1; y1 = y;
            out[f * channels + c] = y;
        }
        s->b_x1[c] = x1; s->b_x2[c] = x2;
        s->b_y1[c] = y1; s->b_y2[c] = y2;
    }
}

static void dsp_process_lowpass(DSA_DSPNode *dsp, float *in, float *out,
                                int frames, int channels) {
    float fc = dsp->params[0] > 1.0f ? dsp->params[0] : 1.0f;
    float w0 = 2.0f * (float)M_PI * fc / DSA_DSP_SR;
    float cosw = cosf(w0), sinw = sinf(w0);
    float q = 1.0f / (1.0f + dsp->params[1]);   /* resonance -> Q */
    float alpha = sinw / (2.0f * q);
    float b0 = (1.0f - cosw) / 2.0f;
    float b1 = 1.0f - cosw;
    float b2 = (1.0f - cosw) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha;
    dsp_biquad(dsp, in, out, frames, channels,
               b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

static void dsp_process_itlowpass(DSA_DSPNode *dsp, float *in, float *out,
                                  int frames, int channels) {
    /* Improved (steeper 2-pole) lowpass, Q from resonance */
    float fc = dsp->params[0] > 1.0f ? dsp->params[0] : 1.0f;
    float w0 = 2.0f * (float)M_PI * fc / DSA_DSP_SR;
    float cosw = cosf(w0), sinw = sinf(w0);
    float q = 0.707f + dsp->params[1] * 2.0f;
    float alpha = sinw / (2.0f * q);
    float b0 = (1.0f - cosw) / 2.0f;
    float b1 = 1.0f - cosw;
    float b2 = (1.0f - cosw) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha;
    dsp_biquad(dsp, in, out, frames, channels,
               b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

static void dsp_process_highpass(DSA_DSPNode *dsp, float *in, float *out,
                                 int frames, int channels) {
    float fc = dsp->params[0] > 1.0f ? dsp->params[0] : 1.0f;
    float w0 = 2.0f * (float)M_PI * fc / DSA_DSP_SR;
    float cosw = cosf(w0), sinw = sinf(w0);
    float q = 0.707f + dsp->params[1];
    float alpha = sinw / (2.0f * q);
    float b0 = (1.0f + cosw) / 2.0f;
    float b1 = -(1.0f + cosw);
    float b2 = (1.0f + cosw) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha;
    dsp_biquad(dsp, in, out, frames, channels,
               b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

static void dsp_process_parameq(DSA_DSPNode *dsp, float *in, float *out,
                                int frames, int channels) {
    /* params: [0]=center Hz, [1]=Q, [2]=gain dB */
    float fc = dsp->params[0] > 1.0f ? dsp->params[0] : 1.0f;
    float w0 = 2.0f * (float)M_PI * fc / DSA_DSP_SR;
    float cosw = cosf(w0), sinw = sinf(w0);
    float q = dsp->params[1] > 0.1f ? dsp->params[1] : 0.1f;
    float A = powf(10.0f, dsp->params[2] / 40.0f);
    float alpha = sinw / (2.0f * q);
    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosw;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha / A;
    dsp_biquad(dsp, in, out, frames, channels,
               b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
}

static void dsp_process_distortion(DSA_DSPNode *dsp, float *in, float *out,
                                   int frames, int channels) {
    float drive = dsp->params[0] > 0.0f ? dsp->params[0] : 1.0f;
    float makeup = dsp->params[1] > 0.0f ? dsp->params[1] : 1.0f;
    int chans = channels > 8 ? 8 : channels;
    for (int f = 0; f < frames; f++) {
        for (int c = 0; c < chans; c++) {
            float x = in[f * channels + c] * drive;
            float y = tanhf(x);          /* soft clip */
            out[f * channels + c] = y * makeup;
        }
    }
}

static void dsp_process_flanger(DSA_DSPNode *dsp, float *in, float *out,
                                int frames, int channels) {
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    if (!s->delay) { dsp_process_passthrough(dsp, in, out, frames, channels); return; }
    float rate = dsp->params[0] > 0.01f ? dsp->params[0] : 0.01f;  /* Hz */
    float depth = dsp->params[1] > 0.0f ? dsp->params[1] : 0.0f;    /* 0..1 */
    float base_ms = 3.0f;
    float wet = dsp->params[2];
    int chans = channels > 2 ? 2 : channels;
    for (int f = 0; f < frames; f++) {
        s->lfo_phase += 2.0 * M_PI * rate / DSA_DSP_SR;
        if (s->lfo_phase > 2.0 * M_PI) s->lfo_phase -= 2.0 * M_PI;
        float lfo = 0.5f + 0.5f * sinf((float)s->lfo_phase);
        int mod = (int)((base_ms + depth * 7.0f) * DSA_DSP_SR / 1000.0f * lfo);
        if (mod < 0) mod = 0;
        if (mod >= s->delay_cap) mod = s->delay_cap - 1;
        for (int c = 0; c < chans; c++) {
            int idx = (s->delay_pos - mod * 2 + c + s->delay_cap) % s->delay_cap;
            float delayed = s->delay[idx];
            float x = in[f * channels + c];
            s->delay[s->delay_pos + c] = x + delayed * 0.6f;
            out[f * channels + c] = x * (1.0f - wet) + delayed * wet;
        }
        s->delay_pos = (s->delay_pos + 2) % s->delay_cap;
    }
}

static void dsp_process_tremolo(DSA_DSPNode *dsp, float *in, float *out,
                                int frames, int channels) {
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    float rate = dsp->params[0] > 0.01f ? dsp->params[0] : 0.01f;
    float depth = dsp->params[1] > 0.0f ? dsp->params[1] : 0.0f;
    int chans = channels > 8 ? 8 : channels;
    for (int f = 0; f < frames; f++) {
        s->lfo_phase += 2.0 * M_PI * rate / DSA_DSP_SR;
        if (s->lfo_phase > 2.0 * M_PI) s->lfo_phase -= 2.0 * M_PI;
        float gain = 1.0f - depth * (0.5f + 0.5f * sinf((float)s->lfo_phase));
        for (int c = 0; c < chans; c++)
            out[f * channels + c] = in[f * channels + c] * gain;
    }
}

static void dsp_process_echo(DSA_DSPNode *dsp, float *in, float *out,
                             int frames, int channels) {
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    if (!s->delay) { dsp_process_passthrough(dsp, in, out, frames, channels); return; }
    float delay_ms = dsp->params[0] > 0.0f ? dsp->params[0] : 1.0f;
    float decay = dsp->params[1] > 0.0f ? dsp->params[1] : 0.0f;
    int delay_samples = (int)(delay_ms * DSA_DSP_SR / 1000.0f);
    if (delay_samples < 1) delay_samples = 1;
    if (delay_samples * 2 >= s->delay_cap) delay_samples = s->delay_cap / 2 - 1;
    int chans = channels > 2 ? 2 : channels;
    for (int f = 0; f < frames; f++) {
        for (int c = 0; c < chans; c++) {
            int idx = (s->delay_pos + delay_samples * 2 + c) % s->delay_cap;
            float delayed = s->delay[idx];
            float x = in[f * channels + c];
            s->delay[s->delay_pos + c] = x + delayed * decay;
            out[f * channels + c] = x * 0.7f + delayed * 0.3f;
        }
        s->delay_pos = (s->delay_pos + 2) % s->delay_cap;
    }
}

/* Schroeder reverb (4 comb + 2 allpass), applied per channel */
static void dsp_reverb_channel(DSA_DSPState *s, int c, float *in, float *out,
                               int frames, float wet) {
    float decay = s->delay ? 0.3f : 0.0f; /* unused placeholder */
    (void)decay;
    for (int f = 0; f < frames; f++) {
        float x = in[f];
        float y = x;
        for (int i = 0; i < 4; i++) {
            int cap = s->comb_cap[i];
            int pos = s->comb_pos[i];
            float delayed = s->comb[c][i][pos];
            s->comb[c][i][pos] = x + delayed * 0.7f;
            y += delayed;
            pos = (pos + 1) % cap;
            s->comb_pos[i] = pos;
        }
        /* 2 allpass */
        for (int i = 0; i < 2; i++) {
            int cap = s->ap_cap[i];
            int pos = s->ap_pos[i];
            float delayed = s->ap[c][i][pos];
            float v = y + delayed * 0.6f;
            s->ap[c][i][pos] = v;
            y = v - delayed;
            pos = (pos + 1) % cap;
            s->ap_pos[i] = pos;
        }
        out[f] = x * (1.0f - wet) + y * 0.25f * wet;
    }
}

static void dsp_process_reverb(DSA_DSPNode *dsp, float *in, float *out,
                               int frames, int channels) {
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    if (!s->comb[0][0]) { dsp_process_passthrough(dsp, in, out, frames, channels); return; }
    float wet = dsp->params[1] > 0.0f ? dsp->params[1] : 0.5f;
    int chans = channels > 2 ? 2 : channels;
    float tmp[4096];
    for (int c = 0; c < chans; c++) {
        for (int f = 0; f < frames; f++) tmp[f] = in[f * channels + c];
        dsp_reverb_channel(s, c, tmp, tmp, frames, wet);
        for (int f = 0; f < frames; f++) out[f * channels + c] = tmp[f];
    }
}

static void dsp_process_compressor(DSA_DSPNode *dsp, float *in, float *out,
                                   int frames, int channels) {
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    float thresh = powf(10.0f, (dsp->params[0] < -60.0f ? -60.0f : dsp->params[0]) / 20.0f);
    float ratio = dsp->params[1] > 1.0f ? dsp->params[1] : 1.0f;
    float makeup = dsp->params[2] > 0.0f ? dsp->params[2] : 1.0f;
    float attack = 0.005f, release = 0.15f;
    float atk = 1.0f - expf(-1.0f / (attack * DSA_DSP_SR));
    float rel = 1.0f - expf(-1.0f / (release * DSA_DSP_SR));
    int chans = channels > 8 ? 8 : channels;
    for (int f = 0; f < frames; f++) {
        float peak = 0.0f;
        for (int c = 0; c < chans; c++) {
            float a = fabsf(in[f * channels + c]);
            if (a > peak) peak = a;
        }
        float env = s->comp_env;
        if (peak > env) env += (peak - env) * atk;
        else            env += (peak - env) * rel;
        s->comp_env = env;
        float gain = 1.0f;
        if (env > thresh)
            gain = thresh / env;                 /* above threshold -> attenuate */
        gain = 1.0f + (gain - 1.0f) / ratio;     /* apply ratio */
        float g = gain * makeup;
        for (int c = 0; c < chans; c++)
            out[f * channels + c] = in[f * channels + c] * g;
    }
}

static void dsp_process_normalize(DSA_DSPNode *dsp, float *in, float *out,
                                  int frames, int channels) {
    float target = dsp->params[0] > 0.0f ? dsp->params[0] : 0.9f;
    int chans = channels > 8 ? 8 : channels;
    float peak = 0.0001f;
    for (int f = 0; f < frames; f++)
        for (int c = 0; c < chans; c++) {
            float a = fabsf(in[f * channels + c]);
            if (a > peak) peak = a;
        }
    float g = target / peak;
    for (int f = 0; f < frames; f++)
        for (int c = 0; c < chans; c++)
            out[f * channels + c] = in[f * channels + c] * g;
}

static void dsp_process_pitchshift(DSA_DSPNode *dsp, float *in, float *out,
                                   int frames, int channels) {
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    if (!s->ps_buf) { dsp_process_passthrough(dsp, in, out, frames, channels); return; }
    float ratio = dsp->params[0] > 0.01f ? dsp->params[0] : 1.0f;
    /* Naive single-buffer resampler: output sample f reads input at f*ratio.
     * Downshift (ratio<1) is exact; upshift (ratio>1) lacks lookahead and is
     * approximate (reads not-yet-written zero tail near buffer end). */
    for (int f = 0; f < frames; f++) {
        float x = in[f * channels];
        s->ps_buf[s->ps_in] = x;
        s->ps_in = (s->ps_in + 1) % s->ps_cap;
        float rp = (float)s->ps_count * ratio;
        int ri = (int)rp % s->ps_cap;
        int rn = (ri + 1) % s->ps_cap;
        float frac = rp - (int)rp;
        float y = s->ps_buf[ri] + (s->ps_buf[rn] - s->ps_buf[ri]) * frac;
        for (int c = 0; c < channels; c++) out[f * channels + c] = y;
        s->ps_count++;
    }
}

static void dsp_process_oscillator(DSA_DSPNode *dsp, float *in, float *out,
                                   int frames, int channels) {
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    (void)in;
    float freq = dsp->params[0] > 0.0f ? dsp->params[0] : 220.0f;
    float amp = dsp->params[1] > 0.0f ? dsp->params[1] : 0.5f;
    int type = (int)dsp->params[2];
    int chans = channels > 8 ? 8 : channels;
    for (int f = 0; f < frames; f++) {
        s->lfo_phase += 2.0 * M_PI * freq / DSA_DSP_SR;
        if (s->lfo_phase > 2.0 * M_PI) s->lfo_phase -= 2.0 * M_PI;
        float v;
        float p = (float)s->lfo_phase / (2.0f * (float)M_PI);
        switch (type) {
        case 1: v = (p < 0.5f) ? 1.0f : -1.0f; break;            /* square */
        case 2: v = 2.0f * p - 1.0f; break;                      /* saw */
        case 3: v = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p); break; /* triangle */
        default: v = sinf((float)s->lfo_phase); break;           /* sine */
        }
        for (int c = 0; c < chans; c++)
            out[f * channels + c] = v * amp;
    }
}

static void dsp_process_passthrough(DSA_DSPNode *dsp, float *in, float *out,
                                    int frames, int channels) {
    (void)dsp;
    memcpy(out, in, sizeof(float) * frames * channels);
}

/* Allocate per-DSP runtime state based on effect type */
void dsa_dsp_bind_process(DSA_DSPNode *dsp) {
    if (!dsp) return;
    DSA_DSPState *s = (DSA_DSPState *)calloc(1, sizeof(DSA_DSPState));
    if (!s) { dsp->process = dsp_process_passthrough; return; }
    dsp->userdata = s;

    int sr = (int)DSA_DSP_SR;
    switch (dsp->type) {
    case DSA_DSP_TYPE_ECHO:
    case DSA_DSP_TYPE_DELAY:
        s->delay_cap = sr * 2;             /* 1s stereo */
        s->delay = (float *)calloc(s->delay_cap, sizeof(float));
        dsp->process = dsp_process_echo;
        break;
    case DSA_DSP_TYPE_FLANGE:
        s->delay_cap = sr * 2;             /* 1s stereo */
        s->delay = (float *)calloc(s->delay_cap, sizeof(float));
        dsp->process = dsp_process_flanger;
        break;
    case DSA_DSP_TYPE_REVERB:
    case DSA_DSP_TYPE_SFXREVERB: {
        int comb_len[4] = {1117, 1357, 1499, 1777};
        for (int i = 0; i < 4; i++) {
            s->comb_cap[i] = comb_len[i];
            for (int c = 0; c < 2; c++)
                s->comb[c][i] = (float *)calloc(comb_len[i], sizeof(float));
        }
        int ap_len[2] = {421, 683};
        for (int i = 0; i < 2; i++) {
            s->ap_cap[i] = ap_len[i];
            for (int c = 0; c < 2; c++)
                s->ap[c][i] = (float *)calloc(ap_len[i], sizeof(float));
        }
        dsp->process = dsp_process_reverb;
        break;
    }
    case DSA_DSP_TYPE_PITCHSHIFT:
        s->ps_cap = sr * 2;
        s->ps_buf = (float *)calloc(s->ps_cap, sizeof(float));
        dsp->process = dsp_process_pitchshift;
        break;
    case DSA_DSP_TYPE_LOWPASS:        dsp->process = dsp_process_lowpass; break;
    case DSA_DSP_TYPE_ITLOWPASS:      dsp->process = dsp_process_itlowpass; break;
    case DSA_DSP_TYPE_HIGHPASS:       dsp->process = dsp_process_highpass; break;
    case DSA_DSP_TYPE_PARAMEQ:        dsp->process = dsp_process_parameq; break;
    case DSA_DSP_TYPE_DISTORTION:     dsp->process = dsp_process_distortion; break;
    case DSA_DSP_TYPE_TREMOLO:        dsp->process = dsp_process_tremolo; break;
    case DSA_DSP_TYPE_COMPRESSOR:     dsp->process = dsp_process_compressor; break;
    case DSA_DSP_TYPE_NORMALIZE:      dsp->process = dsp_process_normalize; break;
    case DSA_DSP_TYPE_OSCILLATOR:     dsp->process = dsp_process_oscillator; break;
    default:                          dsp->process = dsp_process_passthrough; break;
    }
}

void dsa_dsp_free_state(DSA_DSPNode *dsp) {
    if (!dsp || !dsp->userdata) return;
    DSA_DSPState *s = (DSA_DSPState *)dsp->userdata;
    free(s->delay);
    free(s->ps_buf);
    for (int i = 0; i < 4; i++)
        for (int c = 0; c < 2; c++) free(s->comb[c][i]);
    for (int i = 0; i < 2; i++)
        for (int c = 0; c < 2; c++) free(s->ap[c][i]);
    free(s);
    dsp->userdata = NULL;
}
