# XCloud360 — Xbox Cloud Gaming on Xbox 360 (PowerPC / RGH 3)

**An experimental, independent Xbox 360 homebrew project.**

> [!WARNING]
> **This is NOT a working Xbox Cloud Gaming client. Fortnite is NOT playable.**
> Device-code sign-in now has experimental native Xbox 360 TLS/OAuth source, but **no successful sign-in has been verified**. No valid xCloud session, WebRTC media negotiation or game decoding is implemented.
> The latest source changes have **not yet been compiled or tested on real hardware**.

## Mission

Build a native Xbox Cloud Gaming client for the **Xbox 360 Xenon PowerPC**, with the console receiving game video/audio and sending controller input **directly to the cloud**, without an intermediary PC.

**Current release status:** Xbox 360 C++/XDK/XEX prototype with UI, controller support, protocol parser experiments and optional TCP/UDP network diagnostics. Development and runtime testing target **only the real Xbox 360 RGH/RGH 3**. XeLL/libxenon is a separate experimental option, not part of the XEX build.

## What has been verified on the RGH

| Test | Real Xbox 360 result |
| --- | --- |
| Xbox DNS lookup | **1 IPv4 address resolved** |
| TCP Xbox.com port 443 | **Error 10060**, source **8** (`TCP_SELECT`) |
| Direct TCP 1.1.1.1 port 443 | **Error 10060**, source **8** (`TCP_SELECT`) |
| Direct TCP 1.1.1.1 port 80 | New optional test; **no RGH result yet** |
| TLS / Microsoft device-code sign-in | **New experimental source only; not built or validated on RGH** |
| WebRTC / Fortnite | **Not implemented** |

The Xbox DNS resolved a hostname correctly, but outbound TCP connections **timed out**. It is not known whether XNet Retail restrictions, routing or another network limitation caused the failure. Changing DNS alone is not a proven fix. **No cloud connection has been established.**

See [the complete status report](STATUS_DO_PROJETO.md) and [the dated RGH test](docs/TESTE_RGH3_2026-10-20.md).

## New native Xbox 360 interface (awaiting physical-console build)

The diagnostic-first interface was replaced with a controller-driven **game library**. It now provides **two selectable visual themes**, GR33N Neon (dark/green) and DeepPlay Midnight (dark navy/blue). The latter was inspired by studying a separately supplied PS3 DeepPlay PKG, **not by copying its PNGs, PS3 binaries, sounds or fonts**. Both themes draw original Direct3D geometry inside the Xbox 360 XEX.

| Xbox 360 controller | Action |
| --- | --- |
| **LB / RB** | Switch **Games**, **Favourites**, **Settings**, **Tests** tabs |
| **D-pad** | Navigate the 7×3 game grid and select settings |
| **A** | Open an item; on the local demo, reset the score; in Settings, change the selected item |
| **Y** | Mark or unmark Fortnite as a favourite |
| **Left thumbstick / D-pad** | Move the player and collect green targets in the **offline controller demo** |
| **B** | Return from details/tests to the main menu |
| **BACK** | Exit the XEX while no game is streaming |

**Local tests:** switch to the **Tests** tab to run the controller mini-demo, generated video color bars, and offline protocol parsers. Those tests do **not** implement cloud gameplay or H.264 decoding.

**Two graphic themes:** Open **Settings → Visual theme** to switch between **DeepPlay Midnight** (the new default, with independent navy/blue procedural geometry) and **GR33N Neon** (black/green). The XEX does not bundle the supplied PS3 package's fonts, logos, game art or audio. The smoothed internal font is also scaled for 480p rendering, pending testing on physical hardware.

**Network diagnostics are opt-in:** open **Settings → Network test → A**. **Microsoft account login is also opt-in:** open **Settings → Microsoft account → A** for account details, then **A** to request a genuine device code. If TCP/TLS fails, the UI shows diagnostic codes; never type your account password into the XEX. A successful local demo must not be confused with a working Xbox Cloud Gaming session; external connectivity previously failed with `10060` on the physical RGH.

## Feature status

| Feature | Status |
| --- | --- |
| Xbox 360 PowerPC target and older Retail XEX builds | Implemented; older build verified |
| Redesigned Direct3D game library, offline playable demo and controller navigation | Source implemented; new build needs real RGH validation |
| Font embedded in the XEX with bitmap fallback | Source prepared; latest build needs RGH validation |
| DNS, UDP and TCP 80/443 diagnostics | Implemented as probes, **external TCP currently fails on RGH** |
| Local HTTP/JSON/OAuth device-response parsers | Offline tests; integrated Microsoft HTTPS experiment **not validated** |
| RTP parsing and H.264 NAL scanning | Experimental, **not WebRTC/decoding** |
| GR33N-informed XInput gamepad packet encoder | **Source written, uncompiled**; no WebRTC transport |
| Cloud control-message formatting and HandshakeAck parser | **Source written, uncompiled**; no authorized session |
| RTP/H.264 Single NAL, STAP-A, FU-A access-unit assembly | **Source written, uncompiled**; no jitter/NACK, no decoder |
| Xbox-native peer callback bridge | **Source written, uncompiled**; no peer implementation connected |
| Microsoft OAuth device-code flow | **TLS 1.3 and opt-in worker integrated in source; untested on RGH**; no QR code |
| HTTPS/TLS 1.3 implementation | **MIT XboxTLS13 source now in project; real HTTPS/cert verification unverified** |
| xCloud session discovery and Fortnite launch | Not implemented |
| WebRTC/DTLS/SRTP and cloud controller transport | Not implemented |
| Video/audio codec decoders and renderers | Not implemented |
| Playable Fortnite on Xbox 360 | **Unavailable** |

### Unified XboxTLS13 / Microsoft login source (2026-10-20)

The previous separate `XBOXTERMINAL/` proof-of-concept was consolidated into the **single XCloud360 XEX source tree**. Its reusable **MIT-licensed XboxTLS13** engine is under `third_party/xboxtls/`; `src/xboxtls_backend.cpp` bridges certificate-checked HTTPS into `secure_http::Transport`, and `src/xbox_account.cpp` runs Microsoft OAuth Device Code authentication on a worker thread. `src/tls_trust.cpp` embeds the current experimental root store. The Xbox 360 XEX manifest lives at `xex.xml`.

**No compilation or RGH login succeeded in this update.** The last physical network test still returned error `10060`; Xbox Live/XSTS and Xbox Cloud Gaming sessions remain unavailable. The new TLS source also requires an independent security review before real account usage.

The actual public Microsoft application ID remains **local-only** in ignored `auth_config.h`. A new checkout must copy `auth_config.example.h` to `auth_config.h`, then fill in its own registered public client ID. Never commit client secrets, passwords or tokens.

Read the [TLS/login migration notes](docs/TLS_ACCOUNT_INTEGRATION.md) for the exact new files, security assumptions, error diagnostics and next physical-console test.

Full step-by-step development list: **[ROADMAP.md](ROADMAP.md)**.

### Native Xbox 360 port based on GR33N research

The GR33N (PS3) architecture was studied and **selected interoperability components were independently rewritten for Xenon PowerPC/XDK**, rather than compiling PS3 source as an XEX. New files include `src/xcloud_input.cpp`, `src/xcloud_control.cpp`, `src/h264_rtp_receiver.cpp` and `src/xcloud_bridge.cpp`. Fixed offline packet examples have been added to `protocol_selftest.cpp`.

**This is not a complete GR33N port, Xbox Cloud Gaming sign-in, WebRTC implementation or playable Fortnite client.** The latest files have not been compiled on the console. See [GR33N → XEX porting notes](docs/GR33N_XEX_PORT.md) for the exact mapping, remaining dependencies, and GPL-3.0 attribution/licensing considerations.

## Directory layout

```text
XCLOUDFORXBOX360/
├── src/                        # PowerPC C++, UI, network, Microsoft device-code worker
├── docs/
│   ├── REFERENCES.md           # Native-console references and license research
│   ├── GR33N_XEX_PORT.md       # Details of the experimental PS3 architecture adaptation
│   ├── DEEPPLAY_PKG_ANALYSIS.md # Reverse-engineering report for a separate PS3 package
│   ├── POWERPC.md              # Xenon architecture and streaming design
│   ├── RGH_NETWORK.md          # RGH diagnostic results and error sources
│   ├── TESTE_RGH3_2026-10-20.md
│   ├── XELL_ALTERNATIVE.md
│   └── fortnite-architecture.md
├── PASTACODIGOBASE/            # Private PS3 reference sources and old experiments (not in Git)
├── third_party/                # Portable crypto/TLS and JSON libraries
├── assets/Fonts/               # Local XDK font source for embedding (not in Git)
├── tools/xextool/              # Local XEX conversion tool (not in Git)
├── Release/                    # Owner's latest XEX (preserved)
├── ROADMAP.md                  # Full milestone checklist
├── STATUS_DO_PROJETO.md        # Verified / unverified / missing features
├── embed_font.ps1              # Locally generated font data linked into XEX
├── build_release.ps1           # Owner-run build, Retail XEX, no ZIP
├── XCLOUDFORXBOX360.sln
├── XCloud360.vcxproj
└── README.md
```

## Building for the Xbox 360 RGH

Prerequisites on the **developer's computer**:

- Authorized Xbox 360 SDK/XDK and compatible Visual Studio/MSBuild with the `XEDK` environment variable configured.
- XexTool installed **locally**, not committed to the public repository.
- SDK font sources and Bundler, used to generate the internal font privately.
- A real RGH/RGH 3 Xbox 360 for testing.

Before building from a fresh clone, copy the public configuration template to a locally ignored file, and replace the placeholder with **your own registered** Microsoft public Application ID (never a password or client secret):

```powershell
Copy-Item .\auth_config.example.h .\auth_config.h
# Edit auth_config.h and configure your registered client ID.
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_release.ps1
```

The intended output is **`Release\XCloud360.xex`**, in Retail-compatible format. The script generates the font C++ translation unit locally and aims to link the smooth font inside the XEX, with a bitmap font fallback.

> [!NOTE]
> The project owner handles builds and physical-console testing. Recent changes are **uncompiled**. Please report the **first compiler/linker error** with its file and line if a build fails. No ZIP, external font folder or second distribution directory is required by the intended workflow.

Copy the generated XEX to the console and launch through Aurora or XeXMenu. The native game library should open first. Switch to the **Tests** tab with LB/RB for the offline controller/GPU demo or diagnostics. For an **opt-in Microsoft device-code login attempt**, open **Settings → Microsoft account → A → A**; the app must receive a real HTTPS response to display a code. For TCP/DNS probes use **Settings → Network test → A**. No network call runs at startup.

## How we intend to reach Fortnite

1. **Prove outbound TCP and bidirectional UDP on a real RGH.** Current XNet TCP calls time out.
2. If XNet Retail limits external networking, evaluate a separate **XeLL/libxenon/lwIP** backend on the actual Xbox 360.
3. Port modern TLS/DTLS and verify CA certificates and hostnames, with secure entropy and correct clock.
4. Build authorized OAuth Device Code sign-in and verify whether an independent client can access the Xbox Cloud Gaming service.
5. Implement permitted session/entitlement discovery and start an authorized Fortnite session.
6. Implement the **actual negotiated** WebRTC signaling, ICE, DTLS/SRTP, RTP/RTCP and controller channels.
7. Decode audio/video and render using Xbox 360 resources while staying within its CPU and 512 MB shared memory limits.
8. Measure video frame rate and controller latency, stabilize reconnection, and demonstrate legitimate interactive gameplay.

**These steps have not been completed.** There is no guarantee that the current streaming APIs, access rights and hardware capabilities will make direct xCloud gameplay feasible.

## Root folder cleanup (2026-10-20)

The project root has been simplified: obsolete Visual Studio caches (`.sdf`, `.suo`, object/intermediate files, `bin/`) were removed; the old `dist/` XEX was **archived**, and the separate XeLL/libxenon experiment was moved to `PASTACODIGOBASE/XBOX360_EXPERIMENTOS/experimental_xell/`. The active `Release/` was **not touched**. Required build inputs (embedded font generator, local `assets/Fonts/`, `obj/generated/`, ATG framework static library and XexTool) remain in place.

**About 63.8 MiB of disposable build/IDE cache was removed.** `PASTACODIGOBASE/` remains ignored by Git and is not a build dependency. The source mirror excludes archived PS3/PS4 media and the XeLL experiment. See [full cleanup inventory](docs/ROOT_CLEANUP_2026-10-20.md).

## Source organization (2026-10-20)

The old standalone `XBOXTERMINAL/` was removed **after** the reusable, MIT-licensed TLS 1.3 sources were integrated into `third_party/xboxtls/` and the Microsoft Device Code UI/worker into `src/`. The main Xbox 360 XDK project references the native sources directly. **Compilation and actual RGH sign-in have not yet been validated.**

All local PS3 reference materials are in **`PASTACODIGOBASE/`**: `GR33N-main/` (including the separately supplied DeepPlay PS3 PKG), `research_pkg/` (private previews), and `research_tools/` (the inspector). The unused standalone **XeLL/libxenon boot experiment** also lives under `PASTACODIGOBASE/XBOX360_EXPERIMENTOS/experimental_xell/`; it is neither used by the native XEX nor exported to GitHub. No local PS4 checkout was present; external PS4 project links remain in the references. **PASTACODIGOBASE is ignored by Git and not part of the GitHub source mirror.** It is never needed to build the Xbox 360 XEX.

The public-source folder **`XCloud360-GitHub-Source/`** contains only independently publishable project source, docs, manifest, build scripts and required third-party code/licenses. Run `./package_github.ps1` (PowerShell) to resynchronize that folder; the script **does not produce a ZIP** or push commits to GitHub.

See [source reorganization notes](docs/UPDATE_SOURCE_ORGANIZATION.md) and [TLS/login milestone](docs/RELEASE_NOTES_UNIFIED_TLS.md).

## Research references

The five community projects below were reviewed as **references**, not copied source. A user-supplied local PS3 package was also examined independently: [DeepPlay PKG analysis](docs/DEEPPLAY_PKG_ANALYSIS.md). Its SFO identifies **DeepPlay**, a different application from GR33N. The installed PS3 binaries are **not** native Xbox 360 source code, and none of its copyrighted media are redistributed.



- [XStreamingDesktop](https://github.com/Geocld/XStreamingDesktop) — desktop xCloud/remote-streaming architecture, controller/rumble UX.
- [XCTV Player](https://github.com/kjbaker-uk/xctvplayer) — gamepad-first fullscreen Google TV experience; relies on Android/WebView technology unavailable on Xbox 360.
- [xbox_launcher](https://github.com/LuanRoger/xbox_launcher) — Xbox Series-inspired dashboard design; implemented using Flutter and Windows WebView2.
- [XCloud4](https://github.com/asp11edgr/XCloud4) — PS4 native approach; device-code authentication and streaming architecture.
- [GR33N](https://github.com/actuallyniaxx/GR33N) — PS3 native xCloud client; especially relevant for endianness and ports of mbedTLS, libSRTP, usrsctp, libpeer and fixed-point Opus.

Research notes, architecture comparisons and **license obligations**: [docs/REFERENCES.md](docs/REFERENCES.md).

## Security, source ownership and licensing

- Do not enter Microsoft passwords into TXT files or the application. A valid Device Code screen must use **Microsoft's own authorized sign-in flow**.
- Do not disable TLS certificate verification or hardcode another app's credentials/registration identifiers.
- Do not commit tokens, refresh tokens, SDK binaries, Microsoft XPR/font resources, generated font C++ bytes, CPU keys, key vaults or proprietary game files.
- GR33N and XCloud4 are GPLv3 projects, XCTV Player is GPLv2, XStreamingDesktop is MIT. License compatibility must be assessed before copying any source code.
- Choose a license for the original XCloud360 source before redistributing combined/ported code.

This is an independent homebrew effort and is not affiliated with, endorsed by, or supported by Microsoft, Xbox, Sony or Epic Games.
