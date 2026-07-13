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

1. Binary analysis of `.ojd` format  ← **progress below**
2. Lua C API binding signatures (tolua++ or custom?)
3. Network protocol documentation
4. Field format reverse engineering  ← **progress below**
5. Memory allocator analysis  ← **progress below (tracer data)**

---

## RE Session — 2026-07-13 (DataTrace tracer + static)

### `.pak` confirmed as plain ZIP
Verified with `zipfile`. Most entries **stored** (method 0); shaders are
**deflate** (method 8). Concrete structure per archive:

| Archive | Entries | Notable contents |
|---|---|---|
| `scripts.pak` | 24 | `card_models/<name>/<n> - <layer>.lua` + `model.json` + `components/*.lua` |
| `gui.pak` | 104 | `main_menu/css/style.css`, `main_menu/img/*.png` (RmlUi HTML/CSS) |
| `shaders.pak` | 58 | `*.glsl` (deflate-compressed) |
| `fields.pak` | 80 | `field_<name>.fld` board layouts |
| `strings.pak` | 3 | `version.ojtxt`, `credits.ojtxt`, `data.ojd` (1.34 MB) |
| `define.pak` | 1728 | localized `cards.txt`, `units.txt`, `campaign/*.txt` (per-lang dirs: `chs`, …) |

### Lua card-model system (concrete)
`card_models/default/model.json` defines a **layered render pipeline**:
```json
{ "layers": [ { "file":"0 - common", "type_cache":false, "render":false },
               { "file":"1 - bg.lua", "type_cache":true, "render":true, "entry_point":"card_bg_render" },
               { "file":"2 - fg.lua", "render":true, "entry_point":"card_fg_render" } ] }
```
Each layer is a Lua file exporting an `entry_point` function. Lua API observed:
```lua
include("common/cards.lua")
CARD_RECT = bake_rect(x, y, w, h, fill, border_width, border)
  -- fill: color "#aarrggbb"  OR  { LINEAR_GRAD = "name" }
  -- border: { WIDTH=, COLOR="#" }
CARD_RECT.BG = { { IMAGE="system/orange_bw.png", X, Y, SIZE, OPACITY }, ... }
color_argb("#ffffffff")   -- returns color value
```
Conclusion: card visuals = stacked Lua-driven layers (bg/fg) over a base rect;
component system under `components/*.lua` (description, level_cost, …).

### `.fld` field format (binary, NOT encrypted)
`field_highway.fld` = 1800 bytes. Little-endian `uint32` fields. Pattern:
leading 8 zero bytes, then repeated `29 00 00 00` (= 0x29 = **41**) at offsets
8, 16, 32, 40. Hypothesis: 41 = grid dimension (or a tile/connection type id);
header + per-cell/per-tile uint32 structs. Not encrypted (mostly small ints/zeros).

### `data.ojd` — encrypted / custom binary (open target)
1.34 MB, first bytes `ed ce 8a 22`. **Not** zlib / raw-deflate / gzip
(all fail header check). First 4 KB has **256/256 distinct byte values** →
max entropy ⇒ stream-cipher / XOR encrypted or custom codec, not plaintext,
not standard compression. Decryption key/scheme still unknown.

### Memory lifecycle (DataTrace LD_PRELOAD tracer)
Ran the binary headless; captured **14.3 M events / 2.26 GB** in ~120 s
(asset-load phase, no network in this window). Event mix:

| type | count | meaning |
|---|---|---|
| 4097 | 2,445,520 | malloc |
| 4099 | 152,917   | calloc |
| 5    | 264,145   | realloc |
| 2    | 2,000,171 | free |
| 4    | 9,507,474 | memcpy |

No `recvfrom`/`sendto` (type 7/6) ⇒ no network traffic during load.

**Allocation size distribution** (sample of 107,636 allocs from early phase):
- 99 % are tiny: 1–63 B (57 %) and 64–255 B (42 %)
- larger buckets rare (256 B–1 M B: <0.1 % combined)
- ⇒ heap dominated by **small per-object allocations** = Lua 5.4 VM
  (TString/TValue/GC objects) + RmlUi parser nodes + containers.
  Default **libc malloc** services the scripting/UI layer (no custom arena
  visible at the malloc boundary).

**Large media buffers**: across the full trace, **73 allocations > 1 MB**,
up to **19.5 MB** (×3). Repeated identical sizes (10.5 MB ×3, 7.1 MB ×3,
6.1 MB ×3, 5.4 MB ×5) ⇒ canned decode buffers (decompressed OGG/Opus audio
frames and/or texture mip atlases). This is the *second* allocation phase:
small scripting objects first, then a burst of large media decode buffers.

> Note: the game ignores SIGTERM and blocks in D-state I/O while reading the
> multi-GB `.pak` files, so a controlled short trace requires `timeout -s KILL`
> + process-group kill; full runs produce multi-GB event logs.

### Engine identity: **LUNA** (raylib fork)
Runtime log reveals the engine is **LUNA** built on **raylib** ("Initializing raylib Juice版", "LUNA" module tags). So the engine is a custom **raylib** fork, not RmlUi for the core 3D/render (RmlUi may still back some UI). Display initializes fine headless on `:0` (1280×720). Online/launcher uses Steam API + EOS + Discord GameSDK.

### `data.ojd` decryption — status: ENCRYPTED, key NOT yet recovered
**Confirmed facts:**
- `data.ojd` is stored **uncompressed** (zip method 0) inside `strings.pak` → the
  1.34 MB file is the *encrypted* byte stream as-is (no inflate step for this entry).
- The virtual path uses a `{}` prefix: binary references `{}str/data.ojd`; the
  `{}` prefix marks an **encrypted** asset (sibling `version.ojtxt`/`credits.ojtxt`
  in the same pak are plaintext and have no `{}`). Generic VFS read of a `{}` path
  triggers decryption.
- Linked compression codecs (strings): **miniz/deflate**, **bzip2**, **LZMA/XZ**
  (`7zXZ`, `7ZwZwZwZw`). No obvious AES/RC4 S-box found in a quick scan.
- The loader `0x1426830` → helper `0x14275f0` computes `std::hash` (`_ZSt11_Hash_bytes`)
  over the loaded buffer and `bcmp`s it → **integrity hash check** before use.
  Mismatch prints `Couldn't load data.ojd in strings.pak, the game will now
  peacefully close.` and `exit()`s.
- Actual decryption is dispatched **virtually** (vtable: `call *0x18(%r14)` at
  `0x1426820`) through the VFS object → not trivially locatable by linear xref.

**Key-recovery attempts (all failed → rules out simple schemes):**
- zlib / gzip / raw-deflate / bzip2 / XZ / raw-LZMA: all fail to decompress.
- First bytes `ed ce 8a 22 70 78 ea 1d …`; full-buffer entropy max (256/256 distinct
  in first 4 KB) ⇒ encrypted/high-entropy, not plaintext, not standard compression.
- Brute force XOR with key length 1, 2, 3 (ASCII/JSON/zlib-magic heuristics): **0 hits**.
- Candidate string keys (`data.ojd`, `OrangeJuice`, `Luna`, `ojd`, `100OrangeJuice`, …):
  all yield ~0.40 printable ratio, no valid stream.
- Byte-wise `xor %cl,(mem)` loops exist in the binary but belong to string/number
  parsers (`cmp $0x2d`/`-`, `cmp $0x2b`/`+`), not asset decryption. The many
  `xor %reg,(base,index,8)` (8-byte) sites are hash/obfuscation, not a byte cipher.

**Conclusion:** `data.ojd` is protected by a **keyed cipher** (stream/PRNG-XOR or
block) whose key lives in the virtual VFS decrypt method. Recovering it requires
either (a) locating that vtable-resolved method and extracting the key/algorithm
from its disassembly, or (b) a runtime memory dump of the *decrypted* buffer
(capture is hard: decrypt is in-place, and the game blocks on missing large `.pak`
files before/around loading data.ojd in a headless setup).

### Open sub-targets (refined)
1. **data.ojd key**: find the VFS decrypt vtable method; extract key/algorithm.
   Fallback: dump decrypted buffer at runtime (need game to reach data.ojd load —
   supply all `.pak` and capture heap, or hook the virtual read method directly).
2. **`.fld` parser**: confirm `0x29`=41 grid; map cell/connection struct across 80 fields.
3. **Lua C bindings**: tolua++ vs custom; recover signatures (engine = LUNA/raylib,
   Lua 5.4.6 still the scripting layer per earlier scripts).

