/* ALSA output backend — plays mixed audio to default device */

#include "dsa_internal.h"
#include <alsa/asoundlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* Mixer tick fills this buffer, ALSA writes it */
static DSA_OutputBackend *g_out = NULL;
static pthread_t g_thread;
static DSA_SystemData *g_sys = NULL;

static void *alsa_thread_func(void *arg) {
    (void)arg;
    while (g_out && g_out->running) {
        dsa_output_mix_and_write(g_sys, g_out);
    }
    return NULL;
}

int dsa_output_init(DSA_SystemData *sys, DSA_OutputBackend *out) {
    if (!sys || !out) return -1;
    memset(out, 0, sizeof(*out));

    out->samplerate = sys->samplerate;
    out->channels = 2;
    out->buffer_frames = sys->buffer_length;

    /* Allocate mix buffer */
    out->mix_buf = calloc(out->buffer_frames * out->channels, sizeof(float));
    if (!out->mix_buf) return -1;

    /* Open ALSA device */
    snd_pcm_t *pcm;
    int r = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (r < 0) {
        dsa_log( "[dsa] ALSA open: %s\n", snd_strerror(r));
        free(out->mix_buf);
        out->mix_buf = NULL;
        return -1;
    }

    /* Set hardware params */
    snd_pcm_hw_params_t *hw;
    snd_pcm_hw_params_malloc(&hw);
    snd_pcm_hw_params_any(pcm, hw);
    snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, hw, out->channels);
    snd_pcm_hw_params_set_rate_near(pcm, hw, (unsigned int *)&out->samplerate, NULL);

    snd_pcm_uframes_t buf_frames = out->buffer_frames;
    snd_pcm_hw_params_set_buffer_size_near(pcm, hw, &buf_frames);
    snd_pcm_hw_params(pcm, hw);
    snd_pcm_hw_params_free(hw);

    out->handle = pcm;
    dsa_log( "[dsa] ALSA opened: %dHz %dch %luframes\n",
            out->samplerate, out->channels, (long)buf_frames);
    out->buffer_frames = buf_frames;
    return 0;
}

int dsa_output_start(DSA_SystemData *sys, DSA_OutputBackend *out) {
    if (!out || !out->handle) return -1;
    g_out = out;
    g_sys = sys;
    out->running = 1;

    if (pthread_create(&g_thread, NULL, alsa_thread_func, NULL) != 0) {
        out->running = 0;
        return -1;
    }
    return 0;
}

void dsa_output_stop(DSA_OutputBackend *out) {
    if (!out) return;
    out->running = 0;
    pthread_join(g_thread, NULL);

    if (out->handle) {
        snd_pcm_drain((snd_pcm_t *)out->handle);
        snd_pcm_close((snd_pcm_t *)out->handle);
    }
    free(out->mix_buf);
    memset(out, 0, sizeof(*out));
    g_out = NULL;
    g_sys = NULL;
}

void dsa_output_mix_and_write(DSA_SystemData *sys, DSA_OutputBackend *out) {
    if (!sys || !out || !out->handle) return;

    float *mix_buf = (float *)out->mix_buf;
    int frames = out->buffer_frames;
    int channels = out->channels;

    /* Clear mix buffer */
    memset(mix_buf, 0, sizeof(float) * frames * channels);

    /* Run mixer — fills mix_buf with float samples [-1..1] */
    dsa_mixer_tick(sys, mix_buf, frames, channels);

    /* Convert float → S16 and write to ALSA */
    short alsa_buf[8192];
    int write_frames = frames;
    if (write_frames > 4096) write_frames = 4096;

    for (int i = 0; i < write_frames; i++) {
        for (int c = 0; c < channels && c < 2; c++) {
            float s = mix_buf[i * channels + c];
            if (s > 1.0f) s = 1.0f;
            if (s < -1.0f) s = -1.0f;
            alsa_buf[i * channels + c] = (short)(s * 32767);
        }
    }

    snd_pcm_t *pcm = (snd_pcm_t *)out->handle;
    int r = snd_pcm_writei(pcm, alsa_buf, write_frames);
    if (r == -EPIPE) {
        snd_pcm_prepare(pcm);
        snd_pcm_writei(pcm, alsa_buf, write_frames);
    } else if (r < 0) {
        dsa_log( "[dsa] ALSA write: %s\n", snd_strerror(r));
    }
}
