/* DSA example — exercises all major API paths */

#include "dsa.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    DSA_System *sys = NULL;
    int r;

    printf("=== DSA_System_Create ===\n");
    r = DSA_System_Create(&sys, 0x00020306);
    printf("Create: ret=%d sys=%p\n", r, (void*)sys);
    if (r != 0 || !sys) return 1;

    printf("=== Init ===\n");
    r = DSA_System_Init(sys, 32, DSA_INIT_NORMAL, NULL);
    printf("Init: ret=%d\n", r);
    if (r != 0) return 1;

    unsigned int ver = 0;
    DSA_System_GetVersion(sys, &ver);
    printf("Version: 0x%08x\n", ver);

    int numdrivers = 0, curdriver = 0;
    DSA_System_GetNumDrivers(sys, &numdrivers, &curdriver);
    printf("Drivers: %d (current=%d)\n", numdrivers, curdriver);

    char drvname[128];
    DSA_System_GetDriverInfo(sys, 0, drvname, sizeof(drvname), NULL, NULL, NULL, NULL);
    printf("Driver 0: %s\n", drvname);

    r = DSA_System_SetDSPBufferSize(sys, 512, 4);
    printf("SetDSPBufferSize: ret=%d\n", r);

    r = DSA_System_Set3DSettings(sys, 1.0f, 1.0f, 1.0f);
    printf("Set3DSettings: ret=%d\n", r);

    printf("=== CreateSoundGroup ===\n");
    DSA_SoundGroup *sg = NULL;
    r = DSA_System_CreateSoundGroup(sys, "test_group", &sg);
    printf("CreateSoundGroup: ret=%d sg=%p\n", r, (void*)sg);
    if (r == 0 && sg) {
        DSA_SoundGroup_SetVolume(sg, 0.8f);
        float sv = 0;
        DSA_SoundGroup_GetVolume(sg, &sv);
        printf("  SoundGroup volume: %.2f\n", sv);

        int np = 0;
        DSA_SoundGroup_GetNumPlaying(sg, &np);
        printf("  NumPlaying: %d\n", np);
    }

    printf("=== CreateSound (generates 440Hz sine wave) ===\n");
    DSA_Sound *sound = NULL;
    r = DSA_System_CreateSound(sys, "test_sine", DSA_DEFAULT, NULL, &sound);
    printf("CreateSound: ret=%d sound=%p\n", r, (void*)sound);

    if (r == 0 && sound) {
        int mode = 0;
        DSA_Sound_GetMode(sound, &mode);
        printf("  mode: 0x%x\n", mode);

        unsigned int len = 0;
        DSA_Sound_GetLength(sound, &len, DSA_TIMEUNIT_MS);
        printf("  length: %u ms\n", len);

        DSA_SoundInfo info;
        memset(&info, 0, sizeof(info));
        DSA_Sound_GetInfo(sound, &info);
        printf("  info: %dch %dbits %dHz len=%u samples\n",
               info.channels, info.bits_per_sample, info.samplerate, info.length);
    }

    printf("=== PlaySound ===\n");
    DSA_Channel *channel = NULL;
    r = DSA_System_PlaySound(sys, sound, NULL, 0, &channel);
    printf("PlaySound: ret=%d channel=%p\n", r, (void*)channel);

    if (r == 0 && channel) {
        r = DSA_Channel_SetVolume(channel, 0.5f);
        r = DSA_Channel_SetPitch(channel, 1.0f);
        r = DSA_Channel_SetPan(channel, 0.0f);

        int playing = 0;
        DSA_Channel_IsPlaying(channel, &playing);
        printf("  IsPlaying: %d\n", playing);

        float freq = 0;
        DSA_Channel_GetFrequency(channel, &freq);
        printf("  Frequency: %.1f\n", freq);

        unsigned int pos = 0;
        DSA_Channel_GetPosition(channel, &pos, DSA_TIMEUNIT_MS);
        printf("  Position: %u ms\n", pos);
    }

    printf("=== GetMasterChannelGroup ===\n");
    DSA_ChannelGroup *master = NULL;
    r = DSA_System_GetMasterChannelGroup(sys, &master);
    printf("GetMasterChannelGroup: ret=%d master=%p\n", r, (void*)master);

    r = DSA_ChannelGroup_SetVolume(master, 1.0f);
    printf("  SetVolume(1.0): ret=%d\n", r);

    float gv = 0;
    DSA_ChannelGroup_GetVolume(master, &gv);
    printf("  GetVolume: %.2f\n", gv);

    int nc = 0;
    DSA_ChannelGroup_GetNumChannels(master, &nc);
    printf("  NumChannels: %d\n", nc);

    printf("=== CreateDSP ===\n");
    DSA_DSP *lowpass = NULL;
    r = DSA_System_CreateDSPByType(sys, DSA_DSP_TYPE_LOWPASS, &lowpass);
    printf("CreateDSP(LOWPASS): ret=%d dsp=%p\n", r, (void*)lowpass);

    if (r == 0 && lowpass) {
        char name[64];
        unsigned int dspver = 0;
        int dspch = 0;
        DSA_DSP_GetInfo(lowpass, name, &dspver, &dspch, NULL, NULL);
        printf("  DSP info: %s ver=%u ch=%d\n", name, dspver, dspch);

        DSA_DSP_SetParameterFloat(lowpass, 0, 500.0f); /* cutoff */
        DSA_DSP_SetParameterFloat(lowpass, 1, 1.0f);    /* resonance */

        float v0 = 0, v1 = 0;
        DSA_DSP_GetParameterFloat(lowpass, 0, &v0);
        DSA_DSP_GetParameterFloat(lowpass, 1, &v1);
        printf("  params: cutoff=%.1f resonance=%.1f\n", v0, v1);

        int ni = 0, no = 0;
        DSA_DSP_GetNumInputs(lowpass, &ni);
        DSA_DSP_GetNumOutputs(lowpass, &no);
        printf("  inputs=%d outputs=%d\n", ni, no);

        printf("=== AddDSP to master ===\n");
        DSA_DSPConnection *conn = NULL;
        r = DSA_ChannelGroup_AddDSP(master, lowpass, &conn);
        printf("  AddDSP: ret=%d conn=%p\n", r, (void*)conn);

        if (r == 0 && conn) {
            DSA_DSPConnection_SetMix(conn, 0.5f);
            float mix = 0;
            DSA_DSPConnection_GetMix(conn, &mix);
            printf("  conn mix: %.2f\n", mix);
        }
    }

    printf("=== Create more sounds ===\n");
    for (int i = 0; i < 5; i++) {
        DSA_Sound *s;
        if (DSA_System_CreateSound(sys, "extra", DSA_DEFAULT, NULL, &s) == 0) {
            DSA_Channel *ch;
            DSA_System_PlaySound(sys, s, NULL, 0, &ch);
        }
    }
    DSA_ChannelGroup_GetNumChannels(master, &nc);
    printf("  Total channels after 5 more: %d\n", nc);

    DSA_ChannelGroup_Stop(master);
    printf("  Master stop: OK\n");

    printf("=== System_Update (triggers tick) ===\n");
    for (int i = 0; i < 5; i++) {
        r = DSA_System_Update(sys);
        if (r != 0) { printf("  Update[%d] error: %d\n", i, r); break; }
    }
    printf("  Updates: OK\n");

    printf("=== Release ===\n");
    r = DSA_System_Release(sys);
    printf("Release: ret=%d\n", r);

    printf("=== Done ===\n");
    return 0;
}
