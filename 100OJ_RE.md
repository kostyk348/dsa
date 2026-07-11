# 100 Orange Juice — Full Engine Reverse Engineering

## Binary Info

- Path: `~/.local/share/Steam/steamapps/common/100 Orange Juice/100orange`
- Size: 39MB (39053824)
- Type: ELF 64-bit LSB pie executable, x86-64, stripped
- Dynamic libs: steam_api, EOSSDK, discord_game_sdk, FFmpeg, pthread, libstdc++
- Most dependencies statically linked (Lua 5.4.6, SDL2, OpenGL, RmlUi, miniz, etc.)

## Asset Archives (`.pak` = ZIP)

| Archive | Size | Contents |
|---|---|---|
| `audio.pak` | 592MB (18507 files) | BGM (`.ogg`) + voice (`.oga`) per character |
| `graphics.pak` | 1.2GB | Textures, sprites |
| `models.pak` | 27MB | 3D models |
| `videos.pak` | 142MB | Video cutscenes |
| `strings.pak` | 1.4MB | `version.ojtxt`, `credits.ojtxt`, `data.ojd` |
| `define.pak` | 12MB | Card/unit/campaign definitions (`.txt`) |
| `scripts.pak` | 31KB | Card model Lua scripts |
| `gui.pak` | 270KB | HTML/CSS UI (RmlUi) + Lua |
| `fields.pak` | 128KB | Game board layouts (`.fld`) |
| `shaders.pak` | 25KB | GLSL shaders |
| `fonts.pak` | 74MB | TTF/TTC fonts |

## Engine Architecture

### Core Libraries (statically linked)
- **Lua 5.4.6** — game logic scripting (card models, UI, field logic)
- **RmlUi** (libRml) — HTML/CSS UI rendering engine with Lua bindings
- **SDL2** — platform layer (input, window, game controllers, audio)
- **OpenGL** — 3D rendering via GLSL shaders
- **FFmpeg** (libavcodec, libavformat, libavutil, libswresample, libswscale) — media decoding
- **miniz** (or zlib) — ZIP archive reading
- **libass** — subtitle rendering

### Subsystems (from string analysis)

1. **Asset System**: Reads `.pak` ZIP archives from `data/` directory. Mounts by name.
2. **Scripting**: Lua 5.4.6 VM with custom C++ bindings:
   - Card model system (component-based UI layout via `bake_rect()`)
   - Event system (`AddEventListener`)
   - Field background scripts
3. **UI**: RmlUi renders HTML/CSS with Lua bindings (`LuaDocument`, `LuaElementInstancer`, etc.)
4. **Audio**: BGM via `bgmpackstsw` system, voice per character, OGG/Opus format
5. **Video**: FFmpeg-based video playback
6. **Input**: SDL2 game controller with extensive gamepad mappings
7. **Online**: Steam API + Epic Online Services (EOS) + Discord GameSDK
8. **Network**: Custom protocol (packet types: `_PACKET_PLAYER_SLOT_REQUEST`, etc.)

### File Formats

| Format | Description |
|---|---|
| `.pak` | ZIP archive (renamed) |
| `.ojd` | Binary game data (possibly compressed/custom) `data.ojd` in strings.pak |
| `.ojtxt` | Plain text (credits, version) |
| `.fld` | Binary field layout (board cells, positions, connections) |
| `.lua` | Lua 5.4.6 scripts |
| `.html`/`.css` | RmlUi UI markup |
| `.glsl` | OpenGL shaders |

### Lua API (reverse engineered from scripts)

```
include("path")              — include Lua module
bake_rect(x, y, w, h, ...)   — define UI rectangle with color/gradient/border
color_argb("#aarrggbb")      — parse color string
IMAGE = "path.png"           — image reference
LINEAR_GRAD = "name"         — gradient reference
```

## Open Targets

1. Binary analysis of `.ojd` format
2. Lua C API binding signatures (tolua++ or custom?)
3. Network protocol documentation
4. Field format reverse engineering
5. Memory allocator analysis
