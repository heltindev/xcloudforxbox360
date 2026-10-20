# XCloud360 — Xbox Cloud Gaming on Xbox 360 (PowerPC / RGH 3)

**An experimental, independent Xbox 360 homebrew project.**

> [!WARNING]
> **This is NOT a working Xbox Cloud Gaming client. Fortnite is NOT playable.**
> No Microsoft sign-in, valid cloud session, WebRTC media negotiation or real-time game decoding is implemented.
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
| Modern TLS / OAuth / WebRTC / Fortnite | **Not implemented** |

The Xbox DNS resolved a hostname correctly, but outbound TCP connections **timed out**. It is not known whether XNet Retail restrictions, routing or another network limitation caused the failure. Changing DNS alone is not a proven fix. **No cloud connection has been established.**

See [the complete status report](STATUS_DO_PROJETO.md) and [the dated RGH test](docs/TESTE_RGH3_2026-10-20.md).

## Xbox 360 UI

Inspired by Xbox dashboard conventions and [xbox_launcher](https://github.com/LuanRoger/xbox_launcher): fullscreen, dark panels, Xbox green accents and controller-first navigation. No artwork or code from other projects was copied.

| Controller | Action |
| --- | --- |
| **A** | View the roadmap; repeat a test only when inside the diagnostic screen |
| **Y** | Show Fortnite availability and missing dependencies |
| **X** | Start **optional** network diagnostics |
| **B** | Return to the home screen |
| **BACK** | Exit the homebrew application |

**The network diagnostic is not required to open the app.** However, a working network transport will ultimately be required to stream any cloud game.

## Feature status

| Feature | Status |
| --- | --- |
| Xbox 360 PowerPC target and older Retail XEX builds | Implemented; older build verified |
| Direct3D interface and controller navigation | Implemented; latest update needs RGH validation |
| Font embedded in the XEX with bitmap fallback | Source prepared; latest build needs RGH validation |
| DNS, UDP and TCP 80/443 diagnostics | Implemented as probes, **external TCP currently fails on RGH** |
| Local HTTP/JSON/OAuth device-response parsers | Experimental, **offline only** |
| RTP parsing and H.264 NAL scanning | Experimental, **not WebRTC/decoding** |
| Microsoft OAuth sign-in and QR code | Not implemented |
| Verified HTTPS/TLS | Not implemented |
| xCloud session discovery and Fortnite launch | Not implemented |
| WebRTC/DTLS/SRTP and cloud controller transport | Not implemented |
| Video/audio codec decoders and renderers | Not implemented |
| Playable Fortnite on Xbox 360 | **Unavailable** |

Full step-by-step development list: **[ROADMAP.md](ROADMAP.md)**.

## Directory layout

```text
XCLOUDFORXBOX360/
├── src/                        # PowerPC C++, UI, network probes and protocol parsers
├── docs/
│   ├── REFERENCES.md           # Native-console references and license research
│   ├── POWERPC.md              # Xenon architecture and streaming design
│   ├── RGH_NETWORK.md          # RGH diagnostic results and error sources
│   ├── TESTE_RGH3_2026-10-20.md
│   ├── XELL_ALTERNATIVE.md
│   └── fortnite-architecture.md
├── experimental_xell/          # Isolated boot-only libxenon example (no ELF built)
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

Run, in the root project directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_release.ps1
```

The intended output is **`Release\XCloud360.xex`**, in Retail-compatible format. The script generates the font C++ translation unit locally and aims to link the smooth font inside the XEX, with a bitmap font fallback.

> [!NOTE]
> The project owner handles builds and physical-console testing. Recent changes are **uncompiled**. Please report the **first compiler/linker error** with its file and line if a build fails. No ZIP, external font folder or second distribution directory is required by the intended workflow.

Copy the generated XEX to the console and launch through Aurora or XeXMenu. The optional network test is available by pressing **X**.

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

## Research references

The five community projects below were reviewed as **references**, not copied source:

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
