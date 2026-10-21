# GR33N user interface — native Xbox 360 adaptation

**Reference actually inspected:** the locally supplied `PASTACODIGOBASE/GR33N-main/source/ui.c`, `include/ui.h`, `source/text.c` and `docs/screenshot.png`.

**Porting target:** XCloud360, a native `.xex` for **Xenon PowerPC / Xbox 360 RGH 3**, using Xbox 360 XDK, D3D9 and XInput. This is a **new, independent UI implementation**, not a recompiled PS3 UI or a functioning xCloud session.

## Design observations from GR33N

The original PS3 interface is a 1280 × 720 console-first menu using:

- A fixed **64-pixel top bar**, dark background, bright green title and top navigation tabs.
- Four sections: **Games, Favourites, Settings and Tests**.
- A seven-column, three-row library grid (`GRID_COLS=7`, `GRID_ROWS=3`, 160-pixel covers and 16-pixel horizontal gaps).
- A highlighted game cell with a **green focus outline** and separate game details.
- A 48-pixel footer showing gamepad hints.
- User-configurable controller stick deadzone and repeat while holding the D-pad.
- Diagnostic video test cards and hardware checks independent of xCloud streaming.

The supplied `docs/screenshot.png` shows *actual game artwork fetched or cached by GR33N*. The Xbox 360 port **does not copy PS3 screenshots or copyrighted game covers**. Instead it draws neutral, original geometric placeholders until a legitimate catalog and image loader exist.

## Features now rewritten for Xbox 360

| GR33N concept | Xbox 360 implementation | Actual status |
| --- | --- | --- |
| 1280 × 720 dark theme / green accents | `src/app.cpp`, XDK `platform::draw_rect` / `draw_text` | Source written; not yet compiled on RGH |
| Games / Favourites / Settings / Tests | Four-tab native `App::Tab` navigation | Source written |
| Seven-by-three scrolling-style grid | Fixed 7-column, 3-row layout with focus frame, dynamic active-item count | Source written; only one legitimate offline game listing |
| Actual catalog artwork | Native procedural placeholder cards | Deliberately no third-party cover images included |
| Favourite games | Fortnite can be marked/unmarked with Y | Session-memory only; no disk persistence yet |
| Game detail panel | View title, description, development status and favourite action | Source written |
| Controller navigation and repeat | Xbox 360 XInput and `GetTickCount` repeat scheduling | Source written; test on hardware |
| Settings | Language (PT-BR/English), adjustable deadzone, debug HUD and account/network state | Source written; session-memory only |
| Offline video test card | D3D color bars, greyscale, moving scan line and overscan border | Source written; **not H.264 decoding** |
| Gamepad/controller local test | Direct3D movement demo with scoring | Source written; requires physical-console validation |
| Optional protocol diagnostics | Offline parser checks, no Microsoft requests | Source written |
| Network diagnostics | XNet probes run **only** after opening Settings and selecting Network Test | Legacy RGH error 10060 remains unresolved |

### Xbox 360 controller mapping

| Button | Function |
| --- | --- |
| LB / RB | Previous / next tab |
| D-pad | Navigate 7×3 grid or move settings selection (with held repeat) |
| A | Open selected item; rerun an active offline test |
| Y | Mark/unmark Fortnite as favourite (in library or its details view) |
| B | Leave a detail/test screen and return to the menu |
| BACK | Exit the XEX while **no cloud stream is running** |
| Left thumbstick or D-pad | Play the local input demo |
| Settings: Left/Right | Change language, deadzone or HUD |
| Settings: select Network Test + A | Launch the optional TCP/DNS diagnostic |

**Important:** The GR33N source warns that an actual streaming game's controls must belong to the game; a single B/BACK press must not accidentally close a cloud session. An eventual real streaming mode will need a **held exit combination**. There is **no active cloud stream in this XEX**.

## What is deliberately not ported as working functionality

- GR33N's `catArt`/catalog response machinery, favorite persistence and remote artwork downloads: require authorized HTTPS and a validated image loader.
- Actual Microsoft device-code login, token management and xCloud session creation.
- Native PS3 `cellVdec` decoder, video output, RSX renderer and audio system; none of these PS3 APIs can be linked by the Xbox 360 XDK.
- mbedTLS, libpeer, libSRTP, usrsctp and real media transport: still need platform-specific ports, authentication and network connectivity.
- PS3-specific XMB/PKG resources, avatars, textures or game screenshots.

**The previous hardware result stands:** the RGH resolved 1 Xbox DNS address, but TCP 443 timed out to Xbox.com and 1.1.1.1 with `10060`, source `8`. A redesigned menu cannot solve or bypass the transport requirement for a cloud game.

## Licensing

The supplied GR33N repository is **GPL-3.0**. Its screenshot and original assets are **reference material only**; they are not copied into the XEX or the public source mirror. This Xbox 360 code implements general UI layout ideas with its own drawing and input code.

**If actual upstream GR33N source is directly transplanted**, preserve original credits and comply with GPL-3.0, and review the combination with the proprietary Xbox 360 XDK before distributing anything. Review `PASTACODIGOBASE/GR33N-main/LICENSE` and `PASTACODIGOBASE/GR33N-main/CREDITS.md` locally.

## Testing and packaging policy

The owner compiles and runs on Xbox 360 RGH 3. No compilation, ZIP creation, `Release/` changes, or generation of `XCloud360-RGH3` was carried out in this UI adaptation.

Check real-hardware text legibility, the 720p/480p fallback, LB/RB tabs, green focus outline, D-pad repeat, favorites, language, video test card and controller demo **before** calling any of these verified.

See the complete checklist: [ROADMAP.md](../ROADMAP.md) and [STATUS_DO_PROJETO.md](../STATUS_DO_PROJETO.md).
