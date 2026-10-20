# XCloud360 — Verified status and current limitations

**Target:** Xbox 360 Xenon PowerPC, RGH/RGH 3, with no intermediary PC.  
**Status:** experimental Xbox 360 homebrew application; **Fortnite and Xbox Cloud Gaming do not work yet**.  
**Important:** the latest changes were written but **not compiled or tested by the assistant**. The owner will build and test them on the real console.

## Real hardware report (2026-10-20)

| Field | Owner-reported Xbox 360 result | Interpretation |
| --- | --- | --- |
| Xbox DNS | **1 IPv4 address** | Hostname resolved |
| TCP to Xbox.com:443 | **10060**, source **8** | Timeout while waiting for a connection |
| Direct TCP to 1.1.1.1:443 | **10060**, source **8** | Same timeout without DNS |
| Optional DNS over UDP | No result provided | Unverified |
| TCP to 1.1.1.1:80 | No result provided yet | New optional comparison |

**Source 8** is the application's `TcpProbe::TCP_SELECT` stage. The hostname lookup **did work**. The cause of the outgoing TCP timeouts has **not been isolated**: possible explanations include XNet Retail restrictions, route or firewall behavior, or network configuration. A DNS change alone is not a verified fix.

## Present in source code

| Component | Code written | Real RGH validation |
| --- | --- | --- |
| Native PowerPC/Xbox 360 project and older Retail XEX output | Yes | Older revisions ran |
| Xbox controller input and Direct3D renderer | Yes | Basic navigation on older builds |
| Xbox green tile-based UI, Fortnite status and optional network test | Yes | **Latest revision pending** |
| Smooth font generated privately and linked into XEX | Yes | **Latest revision pending** |
| Bitmap fallback font | Yes | Older revisions ran |
| XNet DNS and socket/TCP diagnostic | Yes | **10060 timeout measured** |
| Google DNS over UDP fallback | Yes | Not confirmed on hardware |
| TCP port 80 comparison | Yes | Not confirmed on hardware |
| HTTP/1.1 request/response helper | Partial | Offline parser checks only |
| JSON fields parser | Partial | Offline parser checks only |
| OAuth Device Code response parsing | Partial | No OAuth request or real login |
| RTP packet header parser | Partial | No authenticated media |
| H.264 Annex B NAL scanner | Partial | **Not an H.264 decoder** |
| Full XInput controller snapshot | Partial | No cloud input messages |
| Protocol parser self-check | Yes | **Latest revision pending** |
| Alternate XeLL/libxenon boot proof | Source example exists | ELF **not compiled** |

## Not working / not implemented

- **No verified outbound TCP/UDP network backend** suitable for streaming on the RGH.
- No modern TLS, CA certificate verification, HTTPS client or secure token exchange.
- No authorized Microsoft OAuth client, device code request, sign-in or QR code screen.
- No confirmed permission to use the service's streaming/session APIs with this application.
- No cloud catalog, Fortnite session creation, authorization, queue management or signaling.
- No negotiated WebRTC peer, ICE/STUN/TURN, DTLS/SRTP or SCTP data channel.
- No codec decoder for incoming cloud video; no audio decoder/output.
- No authenticated upload of Xbox controller input to a cloud session.
- No real playable game, benchmark, stable frame rate or latency figure.

## Work in progress (not yet compiled)

- RGH UI now starts on a status/home page: **A = roadmap, Y = Fortnite status, X = optional network diagnostic**, B = back, BACK = exit.
- The network test does **not** run automatically on startup.
- The alternative XeLL folder is isolated from the main XDK/XEX build.
- In-memory font loader follows the Xbox SDK resource allocation conventions more closely.
- The flat OAuth JSON parser checks complete object syntax and duplicate requested keys.
- [Five external reference projects](docs/REFERENCES.md) were reviewed to guide native TLS/WebRTC research and UI design.

## Required next steps

1. Compile latest `Release / Xbox 360` build and send the first compile/link error, if any.
2. Test UI and font on the physical RGH; verify that optional diagnostics do not block launch.
3. Prove external network connectivity through XNet or a separate XeLL/libxenon networking experiment.
4. Port verified HTTPS/TLS with correct certificates and validate legal/authorized xCloud service access.
5. Implement real login and session negotiation, then media/control transport and decoding.
6. Declare Fortnite functional **only** when the real RGH displays and controls a cloud session.

See [ROADMAP.md](ROADMAP.md) for the full task-by-task checklist and [docs/REFERENCES.md](docs/REFERENCES.md) for source/license research.

**Build policy:** the owner compiles. No ZIP, no automatic compilation, no file changes inside `Release/`, no `XCloud360-RGH3` distribution directory, and no SDK/XPR binaries in the public repository.
