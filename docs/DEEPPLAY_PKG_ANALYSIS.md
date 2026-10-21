# DeepPlay PS3 PKG — Package Reverse-Engineering Report

**Analyzed file (private developer machine):** `PASTACODIGOBASE/GR33N-main/XCLOUDPS3.pkg`  
**Target of the new implementation:** Xbox 360 Xenon PowerPC, native Xbox 360 XDK `.xex`.  
**Package execution:** **Never executed.** The header, encrypted package table, selected local images and PS3 metadata were examined offline.

> **Important correction:** `XCLOUDPS3.pkg` belongs to **DeepPlay**, **NOT to the GR33N source tree**, despite being stored in the local `GR33N-main` directory. The two codebases must not be conflated. Nothing in the metadata alone proves that the packaged PS3 program successfully connects to Xbox Cloud Gaming.

## Package identity — confirmed by decryption and PARAM.SFO

| Field | Value |
| --- | --- |
| Container signature | `7F 50 4B 47` (`\x7FPKG`) |
| Container revision | `0x0000` — non-finalized/development PS3 PKG |
| Platform type | `0x0001` — PlayStation 3 |
| Package bytes | **9,030,816 bytes** |
| Records | **55 file/directory records** |
| Content ID | `HB0001-DEEPPLAY1_00-DEEPPLAYHB000001` |
| SFO title | **DeepPlay** |
| SFO title ID | `DEEPPLAY1` |
| SFO version | `01.00` (app version `00.01`) |
| SFO minimum firmware field | `04.7500` |
| SFO category | `HG` |
| SFO license label | `Homebrew. Not affiliated with Sony Computer Entertainment Inc.` |

### Relevant installed files

| File group | Findings | Meaning for an Xbox 360 client |
| --- | --- | --- |
| `USRDIR/EBOOT.BIN` | **593,520 bytes**, SCE magic `53 43 45 00` | PS3 executable, not an XEX or PowerPC Xbox 360 source tree |
| `USRDIR/plugins/deepplay-ps.sprx` | **10,691 bytes**, SCE-format module | PS3 plugin/runtime dependency, not loadable by the Xbox 360 |
| `PARAM.SFO`, `ICON0.PNG`, `PIC1.PNG`, `SND0.AT3` | PS3 system registration assets | Used by the PS3 XMB/package loader, not by Aurora/XeXMenu |
| `USRDIR/ui/*.png` | 30+ interface assets including library/settings/search icons, badges, welcome and logos | Visual reference only; requires new Xbox-native renderer and rights check |
| `USRDIR/ui/sst-{bold,medium,regular,light}.ttf` | Four SST TrueType font resources | These are not licensed automatically for redistribution; **none are embedded in the XEX** |
| `USRDIR/ui/*.wav`, `music.ogg` | Move/selection/background sounds and music | May inspire UI sound cues; actual port needs Xbox audio API and licensed assets |

### DeepPlay visual observations

The privately inspected `PIC1.PNG` uses a **dark navy-to-indigo gradient**, thin orbit-like curves, scattered points and the PlayStation geometry motif. Other resources include large welcome/library/settings icons and a PlayStation startup mark. **These symbols and PNG files have not been copied into the Xbox 360 project.**

Instead, the new Xbox 360 code adds an **independently drawn, procedural DeepPlay Midnight theme** using D3D rectangles, gradients, blue orbits and geometric forms. The existing **GR33N Neon** theme remains available. The selected theme is an app-local setting, not a PS3 runtime or a copied game asset.

### Source vs. executable: what reverse engineering can establish

- The PS3 PKG item table was decrypted locally using the **non-finalized PKG** SHA-1 stream algorithm, so the installed filenames and sizes are grounded in the actual supplied file.
- The `PARAM.SFO` was parsed locally to confirm the product title, content ID and package metadata.
- Selected UI PNGs were extracted **only** to `PASTACODIGOBASE/research_pkg/visual_only/` for private study. That directory is excluded from Git.
- `EBOOT.BIN` and `deepplay-ps.sprx` were identified as **SCE-format compiled PS3 executables**, not readable C/C++ source code. They were **not** executed, decrypted to native PS3 ELF, decompiled to source, or adapted into an Xbox XEX.
- The application/network protocol logic inside those protected binaries **has not been proven by this inspection**. The available **PASTACODIGOBASE/GR33N-main/source/** tree remains the better source-level reference for networking, xCloud formats and WebRTC architecture.

## What was newly implemented for XCloud360

| Xbox 360 file | Rewritten behavior | Status |
| --- | --- | --- |
| `src/app.cpp` | **DeepPlay Midnight** palette and procedural background, still with original Xbox 360 GR33N-like Games/Favourites/Settings/Tests tabs | C++ source edited; owner compilation/test pending |
| `src/app.cpp` / `src/app.h` | Settings item **Visual theme** switches between DeepPlay Midnight and GR33N Neon without loading PS3 PNG/TTF assets | Source edited, not yet hardware-tested |
| `PASTACODIGOBASE/research_tools/inspect_pkg.ps1` | Local PS3 development-PKG header, file-table, filenames and selected private reference extraction | **Executed successfully on the supplied 9 MB PKG** |
| `docs/DEEPPLAY_PKG_ANALYSIS.md` | This analysis report | Written in English for GitHub |

## Still NOT implemented or validated

1. Loading the real Microsoft-authenticated xCloud game catalog, game covers and avatars.
2. A licensed asset pipeline for game covers, icons, fonts and audio.
3. Xbox 360 system audio notifications or music derived from package assets.
4. Decompiling or porting the PS3 `EBOOT.BIN` or `.sprx` (these require PS3-specific runtime symbols, libraries and potentially additional authorization).
5. Verified external network access (the Xbox 360 RGH last reported TCP **10060**, source **8**, despite DNS resolving one IP).
6. TLS/certificate validation, permitted Microsoft sign-in, xCloud session authorization, WebRTC and H.264/audio decoding on Xbox 360.

**This implementation improves native UI design and our understanding of the supplied PKG. It does not yet make Fortnite playable on the Xbox 360.**

## Reproduce the local analysis

From the root of the developer's checkout:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\PASTACODIGOBASE\research_tools\inspect_pkg.ps1
```

Optional, **private** visual-reference extraction (never commit the output):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\PASTACODIGOBASE\research_tools\inspect_pkg.ps1 -ExtractReferences
```

The script checks the exact supported non-finalized PS3 PKG format before interpreting records; it does not run a PS3 executable or download other software.

### Licensing and publishing

The existence of a homebrew package and a generic SFO license string **does not grant permission to republish all its TTF fonts, sound files, images, binaries or reverse-engineered code**. This project reimplements general design concepts with original geometry and Xbox SDK primitives.

Do not commit `PASTACODIGOBASE/` (containing `GR33N-main/` and `research_pkg/`), Microsoft SDK font resources, any `EBOOT.BIN`, `.sprx`, `.ttf`, extracted PNG/audio, credentials or keys to the public XCloud360 repository.

See [GR33N_XEX_PORT.md](GR33N_XEX_PORT.md), [GR33N_UI_XBOX360.md](GR33N_UI_XBOX360.md), [ROADMAP.md](../ROADMAP.md).
