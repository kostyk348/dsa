#include "dsa_internal.h"
#include <mpg123.h>

/* Detect audio format from magic bytes */
static int detect_format(const unsigned char *data, size_t size) {
    dsa_log( "[dsa-dec] detect: size=%zu bytes=[%02x %02x %02x %02x %02x %02x...] ",
            size, data[0], data[1], data[2], data[3], data[4], data[5]);
    int fmt;
    if (size < 4) fmt = DSA_SOUNDFORMAT_RAW;
    else if (data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F')
        fmt = DSA_SOUNDFORMAT_WAV;
    else if (data[0] == 0xFF && (data[1] & 0xE0) == 0xE0)
        fmt = DSA_SOUNDFORMAT_MP3;
    else if (data[0] == 'O' && data[1] == 'g' && data[2] == 'g')
        fmt = DSA_SOUNDFORMAT_OGG;
    else
        fmt = DSA_SOUNDFORMAT_RAW;
    dsa_log( "fmt=%d (0=raw 1=wav 2=mp3 3=ogg)\n", fmt);
    return fmt;
}

/* Parse WAV/RIFF header and extract PCM data (alloc-free, returns ptr inside buffer) */
static const short *parse_wav(const unsigned char *data, size_t size,
                              unsigned int *out_samples, int *out_channels,
                              int *out_samplerate) {
    if (size < 44) return NULL;
    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F')
        return NULL;
    if (data[8] != 'W' || data[9] != 'A' || data[10] != 'V' || data[11] != 'E')
        return NULL;

    /* Find fmt chunk */
    size_t pos = 12;
    while (pos + 8 <= size) {
        unsigned int chunk_len = (unsigned int)(data[pos+4]) |
                                ((unsigned int)(data[pos+5]) << 8) |
                                ((unsigned int)(data[pos+6]) << 16) |
                                ((unsigned int)(data[pos+7]) << 24);
        if (data[pos] == 'f' && data[pos+1] == 'm' && data[pos+2] == 't' && data[pos+3] == ' ') {
            if (chunk_len >= 16 && pos + 24 <= size) {
                int fmt_tag = data[pos+8] | (data[pos+9] << 8);
                *out_channels   = data[pos+10] | (data[pos+11] << 8);
                *out_samplerate = (int)((unsigned int)(data[pos+12]) |
                                       ((unsigned int)(data[pos+13]) << 8) |
                                       ((unsigned int)(data[pos+14]) << 16) |
                                       ((unsigned int)(data[pos+15]) << 24));
                if (fmt_tag != 1) return NULL;
            }
            pos += 8 + chunk_len;
            break;
        }
        pos += 8 + chunk_len;
        if (chunk_len == 0) break;
    }

    /* Find data chunk */
    while (pos + 8 <= size) {
        unsigned int chunk_len = (unsigned int)(data[pos+4]) |
                                ((unsigned int)(data[pos+5]) << 8) |
                                ((unsigned int)(data[pos+6]) << 16) |
                                ((unsigned int)(data[pos+7]) << 24);
        if (data[pos] == 'd' && data[pos+1] == 'a' && data[pos+2] == 't' && data[pos+3] == 'a') {
            unsigned int data_bytes = chunk_len;
            if (pos + 8 + data_bytes > size)
                data_bytes = (unsigned int)(size - pos - 8);
            *out_samples = data_bytes / (*out_channels * 2);
            return (const short *)(data + pos + 8);
        }
        pos += 8 + chunk_len;
    }
    return NULL;
}

/* Decode MP3 to PCM using libmpg123 */
static short *decode_mp3(const unsigned char *data, size_t size,
                         unsigned int *out_samples, int *out_channels,
                         int *out_samplerate) {
    int ret;
    mpg123_handle *mh = mpg123_new(NULL, &ret);
    if (!mh) return NULL;

    ret = mpg123_open_feed(mh);
    if (ret != MPG123_OK) {
        mpg123_delete(mh);
        return NULL;
    }

    /* Feed all MP3 data at once */
    ret = mpg123_feed(mh, data, size);
    if (ret != MPG123_OK) {
        dsa_log( "[dsa] mpg123_feed failed: %s\n", mpg123_plain_strerror(ret));
        mpg123_delete(mh);
        return NULL;
    }

    /* Decode ALL frames, capturing the first to get format */
    size_t decoded_total = 0;
    size_t decoded_cap = 65536;
    short *decoded = (short *)malloc(decoded_cap);
    if (!decoded) { mpg123_delete(mh); return NULL; }

    long rate = 0;
    int channels = 0;
    int got_format = 0;
    unsigned char buf[8192];
    size_t done;

    while ((ret = mpg123_read(mh, buf, sizeof(buf), &done)) == MPG123_OK ||
           ret == MPG123_NEW_FORMAT) {
        if (!got_format) {
            int enc;
            if (mpg123_getformat(mh, &rate, &channels, &enc) == MPG123_OK) {
                got_format = 1;
            }
        }
        if (done > 0) {
            size_t samples = done / sizeof(short);
            if (decoded_total + samples > decoded_cap / sizeof(short)) {
                while (decoded_total + samples > decoded_cap / sizeof(short))
                    decoded_cap *= 2;
                short *newd = (short *)realloc(decoded, decoded_cap);
                if (!newd) { free(decoded); mpg123_delete(mh); return NULL; }
                decoded = newd;
            }
            memcpy(decoded + decoded_total, buf, done);
            decoded_total += samples;
        }
    }

    /* Try once more if NEW_FORMAT was the exit (format change mid-stream) */
    int safety = 0;
    while (ret == MPG123_NEW_FORMAT && safety < 4) {
        if (!got_format) {
            int enc;
            if (mpg123_getformat(mh, &rate, &channels, &enc) == MPG123_OK)
                got_format = 1;
        }
        ret = mpg123_read(mh, buf, sizeof(buf), &done);
        if (done > 0) {
            size_t samples = done / sizeof(short);
            if (decoded_total + samples > decoded_cap / sizeof(short)) {
                while (decoded_total + samples > decoded_cap / sizeof(short))
                    decoded_cap *= 2;
                short *newd = (short *)realloc(decoded, decoded_cap);
                if (!newd) { free(decoded); mpg123_delete(mh); return NULL; }
                decoded = newd;
            }
            memcpy(decoded + decoded_total, buf, done);
            decoded_total += samples;
        }
        safety++;
    }

    if (!got_format) {
        dsa_log( "[dsa] mpg123: could not determine format\n");
        free(decoded);
        mpg123_delete(mh);
        return NULL;
    }

    *out_samplerate = (int)rate;
    *out_channels = channels;
    *out_samples = (unsigned int)decoded_total;
    mpg123_delete(mh);
    return decoded;
}

/* Main decoder entry point.
 * Given raw file data, decodes to 16-bit PCM.
 * Returns malloc'd buffer (caller frees).
 */
short *dsa_decode_audio(const unsigned char *data, size_t size,
                        unsigned int *out_samples, int *out_channels,
                        int *out_samplerate) {
    if (!data || !size || !out_samples || !out_channels || !out_samplerate)
        return NULL;

    int fmt = detect_format(data, size);
    short *pcm = NULL;

    switch (fmt) {
    case DSA_SOUNDFORMAT_WAV: {
        int ch = 0, sr = 0;
        unsigned int samples = 0;
        const short *wav_pcm = parse_wav(data, size, &samples, &ch, &sr);
        if (wav_pcm) {
            size_t bytes = samples * ch * sizeof(short);
            pcm = (short *)malloc(bytes);
            if (pcm) {
                memcpy(pcm, wav_pcm, bytes);
                *out_samples = samples * ch;
                *out_channels = ch;
                *out_samplerate = sr;
                dsa_log( "[dsa] Decoded WAV: %uch %dHz %u samples\n",
                        ch, sr, samples);
            }
        }
        break;
    }
    case DSA_SOUNDFORMAT_MP3: {
        pcm = decode_mp3(data, size, out_samples, out_channels, out_samplerate);
        if (pcm) {
            dsa_log( "[dsa] Decoded MP3: %uch %dHz %u samples\n",
                    *out_channels, *out_samplerate, *out_samples);
        } else {
            dsa_log( "[dsa] MP3 decode failed, using raw\n");
        }
        break;
    }
    case DSA_SOUNDFORMAT_OGG:
        dsa_log( "[dsa] OGG decode not yet supported\n");
        return NULL;
    case DSA_SOUNDFORMAT_RAW:
    default:
        /* Assume raw 16-bit PCM */
        size_t samples = size / sizeof(short);
        pcm = (short *)malloc(size);
        if (pcm) {
            memcpy(pcm, data, size);
            *out_samples = (unsigned int)samples;
            *out_channels = 1;
            *out_samplerate = 44100;
            dsa_log( "[dsa] Using raw PCM: %u samples\n", (unsigned)samples);
        }
        break;
    }

    return pcm;
}
