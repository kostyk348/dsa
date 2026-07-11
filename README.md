## DSA — Data-oriented Streaming Audio

Open-source FMOD replacement built on DSO patterns:
arena allocators, SoA (Struct of Arrays) layout, tick-aware mix loop.

Drop-in replacement for `libfmod.so` / `libfmodstudio.so` via `LD_PRELOAD`.
Tested on Slay the Spire 2 (Steam).

### Architecture

```
┌────────────────────────────────────────────┐
│  DSA_System  (owns arenas for each type)   │
│  ┌──────────────────────────────────────┐  │
│  │  Arena: "channel"                    │  │
│  │  ├─ volume[]    pitch[]    pan[]     │  │  ← SoA layout
│  │  ├─ frequency[] sound_id[] state[]   │  │
│  │  └─ pos[]       vel[]               │  │
│  ├─ Arena: "sound"      — PCM + metadata │
│  ├─ Arena: "dsp"        — DSP graph nodes│
│  ├─ Arena: "channelgrp" — group trees    │
│  └─ Arena: "soundgroup" — sound groups   │
│                                           │
│  Tick loop: mix_channel() → DSP chain →   │
│  output (ALSA)                            │
└────────────────────────────────────────────┘
```

### DSO Patterns Used

| Pattern | Implementation |
|---|---|
| **Arena allocator** | Per-type bump allocator with 64KB cache-aligned blocks, bulk-free at tick |
| **SoA layout** | Channels stored as parallel float/int arrays for SIMD-friendly mixing |
| **Tick-aware** | Mixer runs on audio callback boundary; arena epoch tracked per tick |
| **Provenance** | Hash-chain audit trail on every allocation |
| **Semantic typing** | Arena `type_name` classifies every allocation by object type |

### DSP Chain

`mix_channel()` runs each channel's DSP graph before summing to the output
buffer. Every DSP node carries its own `DSA_DSPState` (allocated/freed via
`dsa_dsp_bind_process` / `dsa_dsp_free_state`), so effects are stateful and
tick-stable.

15 effects implemented in `src/mixer.c`:

| Effect | Notes |
|---|---|
| lowpass / highpass / itlowpass | one-pole and IT-style resonant filters |
| parameq | parametric EQ (center freq + bandwidth) |
| distortion | waveshaper + output gain |
| flanger | modulated delay (LFO on delay time) |
| tremolo | amplitude LFO |
| echo | feedback delay line |
| delay | single-tap delay |
| reverb / sfxreverb | algorithmic + SFX reverb |
| compressor | peak compressor with threshold/ratio |
| normalize | peak-normalize pass |
| pitchshift | resampling-based pitch shift |
| oscillator | generated waveform source |

### 3D Spatialization

Real 3D positioning in `mix_channel()`:

- **Distance attenuation** — inverse-rolloff model (`min_distance` /
  `max_distance` SoA fields; `DSA_Channel_Set3DMinMaxDistance` writes them).
  Verified curve: near peak ≈ 0.25, 200 m ≈ 0.0025.
- **Azimuth pan** — per-channel pan computed from listener `forward` / `up` /
  `right` vectors and the channel's 3D position.

### Build

Requirements: `gcc`, `make`, `libasound2-dev` (ALSA), `libmpg123-dev` (decoder).

```sh
make           # builds libdsa.so + libfmod.so + libfmodstudio.so + example
LD_LIBRARY_PATH=. ./example   # run test (hear 440Hz sine wave)
```

### Drop-in FMOD Replacement

Replace FMOD in any Linux game without modifying the game:

```sh
DSA_DIR=/path/to/dsa
export LD_LIBRARY_PATH="$DSA_DIR"
export LD_PRELOAD="$DSA_DIR/libfmod.so:$DSA_DIR/libfmodstudio.so"
steam steam://rungameid/<appid>
```

Or use the helper script:

```sh
./steam-with-dsa.sh <appid>
```

Example: Slay the Spire 2 (appid 2861150):
```
FMOD Sound System: Successfully initialized
```

### API Coverage

All core FMOD API functions implemented. Covers:
- **System**: Create/Init/Close/Release/Update/Version/Driver info/3D settings/Advanced settings
- **Sound**: Create/GetMode/SetMode/GetLength/GetInfo/Release
- **Channel**: PlaySound/SetVolume/SetPitch/SetPan/IsPlaying/Stop/3D attributes/Frequency/Position/AddDSP
- **ChannelGroup**: GetMaster/AddDSP/SetVolume/GetPitch/GetNumChannels/GetChannel/Stop
- **DSP**: CreateByType/GetInfo/SetParameter/GetParameter/AddDSP/Remove/SetActive
- **DSPConnection**: SetMix/GetMix
- **SoundGroup**: Create/SetVolume/GetVolume/GetNumPlaying
- **Geometry**: Create/Release
- **Studio System**: Create/Release/Init/Update/LoadBank/GetCoreSystem + C++ ABI

C++ ABI (`_ZN4FMOD*`) symbols exported: **205 symbols** covering:
- `FMOD::System`, `FMOD::Sound`, `FMOD::Channel`, `FMOD::ChannelGroup`
- `FMOD::ChannelControl`, `FMOD::DSP`, `FMOD::DSPConnection`
- `FMOD::Studio::System`, `FMOD::Studio::EventInstance`, `FMOD::Studio::EventDescription`
- `FMOD::Studio::Bank`, `FMOD::Studio::Bus`, `FMOD::Studio::VCA`

### License

MIT
