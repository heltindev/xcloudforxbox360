# XCloud360 — Verified status and current limitations

**Target:** Xbox 360 Xenon PowerPC, RGH/RGH 3, with no intermediary PC.  
**Status:** experimental Xbox 360 homebrew application; **Fortnite and Xbox Cloud Gaming do not work yet**.  
**Important:** the latest changes were written but **not compiled or tested by the assistant**. The owner will build and test them on the real console.

## Root cleanup and source-mirror refresh (2026-10-20)

- Removed old generated Visual Studio, Xbox 360 and ATG intermediate files (about **63.8 MiB**), keeping the required `AtgFramework.lib` and generated embedded-font source.
- Archived redundant `dist/` outputs and the boot-only XeLL experiment under ignored `PASTACODIGOBASE/`; `Release/` and local XexTool/SDK font assets are preserved.
- The native XDK project does not compile archived PS3/PS4 sources or the separate XeLL example.
- `XCloud360-GitHub-Source/` contains the current Xbox 360 source and English documentation, without PS3 binaries, compiled XEX or local auth configuration.
- **No new compile, network test, sign-in or gameplay result** came from this cleanup.
- See [ROOT_CLEANUP_2026-10-20.md](docs/ROOT_CLEANUP_2026-10-20.md).

## Source organization update (2026-10-20)

- **`XBOXTERMINAL/` was removed**; reusable Xbox-native HTTPS/TLS code is maintained under `third_party/xboxtls/`, with new account/frontend adapters under `src/`.
- Original PS3 GR33N source + the distinct DeepPlay PKG, extracted private PS3 assets and the inspector were grouped into `PASTACODIGOBASE/`. These are **research material, excluded from Git**, not XEX build dependencies.
- No local PS4 checkout was found. Public XCloud4 references remain linked as external sources.
- The `XCloud360-GitHub-Source/` mirror is prepared with current Xbox 360 code and documentation, without proprietary PS3 assets, user account configuration or compiled XEX.
- **This does not establish working xCloud networking or sign-in.** Hardware compiler/network validation is still pending.

## Unified native TLS/login update (2026-10-20)

- The standalone `XBOXTERMINAL` proof-of-concept source has been consolidated under `third_party/xboxtls/`, with its MIT license preserved and unnecessary standalone project/binaries removed.
- New native XEX integration: `src/xboxtls_backend.cpp`, `src/xbox_account.cpp`, `src/tls_trust.cpp`, `xex.xml` and local-only Microsoft application-ID configuration.
- The main UI can attempt opt-in Microsoft device-code login in a background thread. It is designed to show real server-issued verification codes only, and to display TLS/WinSock/HTTP errors without closing the app.
- **No latest source was compiled or tested**; no working TLS handshake, account login, Xbox Live/XSTS or cloud streaming was demonstrated. The old Xenia `XBOXTERMINAL.xex` ended with code `3`, indicating an HTTPS-request failure but not isolating its origin.
- Source layout, prerequisites and next checks: [TLS_ACCOUNT_INTEGRATION.md](docs/TLS_ACCOUNT_INTEGRATION.md).

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
| OAuth Device Code parsing + background worker | TLS-backed request and polling wired in source | **No verified Microsoft response/login on RGH** |
| RTP packet header parser | Partial | No authenticated media |
| XInput to cloud controller binary reports | New source exists | **Not compiled; no WebRTC input channel** |
| Cloud handshake/capabilities/feedback message formats | New source exists | **Not compiled; no authorized peer** |
| H.264 RTP FU-A/STAP-A depacketizer and IDR resync | New source exists | **Not compiled; no reorder buffer or decoder** |
| XEX-native callback bridge (media + input) | New source exists | **No live WebRTC implementation** |
| H.264 Annex B NAL scanner | Partial | **Not an H.264 decoder** |
| Full XInput controller snapshot | Partial | No cloud input messages |
| TLS 1.3, trust anchors and HTTPS form transport | Third-party MIT sources + native adapter wired in XDK project | **Not compiled; certificate/hostname validation unverified** |
| Account login panel in main XEX | A starts a worker; shows genuine server code if received, or TLS/WSA/HTTP errors | **Not tested on RGH** |
| Protocol parser self-check | Yes | **Latest revision pending** |
| Alternate XeLL/libxenon boot proof | Source example exists | ELF **not compiled** |

## Not working / not implemented

- **No verified outbound TCP/UDP network backend** suitable for streaming on the RGH.
- No **verified functioning** modern TLS/CA/HTTPS exchange on real RGH. Code is now integrated but compilation, certificate-chain/hostname validation and external network connectivity remain unverified.
- No **successful authorized** Microsoft login has been demonstrated. Device-code worker and on-screen UI now exist in source; a real server response and xCloud permission have not been verified. No QR generation is implemented.
- No confirmed permission to use the service's streaming/session APIs with this application.
- No cloud catalog, Fortnite session creation, authorization, queue management or signaling.
- No negotiated WebRTC peer, ICE/STUN/TURN, DTLS/SRTP or SCTP data channel.
- No codec decoder for incoming cloud video; no audio decoder/output.
- No authenticated upload of Xbox controller input to a cloud session.
- No real playable game, benchmark, stable frame rate or latency figure.

## Work in progress (not yet compiled)

- RGH UI now uses a GR33N-inspired four-tab Games/Favourites/Settings/Tests screen. A separate PS3 DeepPlay PKG was inspected offline, and a new **DeepPlay Midnight** blue/navy theme was added alongside **GR33N Neon** (Settings → Visual theme). D-pad/LB/RB navigate, A opens, Y favorites, B returns and BACK exits. **Source written; owner RGH build/test still pending.**
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
