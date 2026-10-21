# Reference projects and technology transfer plan

> Research notes for **XCloud360**, targeting **Xbox 360 Xenon PowerPC on RGH/RGH 3**. This is not evidence that the console can currently connect to Xbox Cloud Gaming. **No external code has been copied into this repository.**

## Projects reviewed

| Reference | What the repository describes | What we can learn | What cannot be carried over unchanged |
| --- | --- | --- | --- |
| [XStreamingDesktop](https://github.com/Geocld/XStreamingDesktop) | Desktop xCloud/remote-streaming client, Windows/macOS/Linux; gamepads, vibration and 1080p features | Service/API layering, session state, feedback, controller UX | Desktop Node.js/Electron stack, operating-system codecs and WebRTC cannot be compiled as Xbox 360 XEX directly |
| [XCTV Player](https://github.com/kjbaker-uk/xctvplayer) | Fullscreen xCloud-oriented Android TV/Chromecast player using Kotlin and an Android web runtime | 10-foot UI, focused sign-in and gamepad navigation | Android Kotlin/WebView is not available in an Xbox 360 homebrew XEX |
| [xbox_launcher](https://github.com/LuanRoger/xbox_launcher) | Xbox Series-inspired Flutter launcher using Windows WebView2 | **UI design inspiration**: dashboard tile layout, Xbox green accents, focus hierarchy, full-screen game selection | Flutter, WebView2, Windows/Selenium are not native Xenon components |
| [XCloud4](https://github.com/asp11edgr/XCloud4) | PS4 GoldHEN native xCloud client, README describes Device Code sign-in, streaming and error logs | Native console authentication UX, sign-in over a second device, session lifecycle, controller feedback | OpenOrbis/PS4 GPU, video and network APIs do not run on Xbox 360; reported features must be verified in that target, not assumed on ours |
| [GR33N](https://github.com/actuallyniaxx/GR33N) | PS3 homebrew xCloud client using mbedTLS, libSRTP, usrsctp, libpeer, fixed-point Opus | **Most relevant native architecture reference**: PowerPC/endianness considerations, TLS/DTLS, STUN/ICE/WebRTC, audio buffers, controller input and performance | PS3 PSL1GHT/RSX/PPC memory layout are different from Xbox 360 XDK/Xenos; dependency ports and legally compatible licenses are required |

For the **actual source-based architecture study and native Xbox 360 adapters**, see [GR33N_XEX_PORT.md](GR33N_XEX_PORT.md), including the pinned GR33N commit and unimplemented pieces.

A separately supplied `XCLOUDPS3.pkg` was inspected offline and is identified as **DeepPlay** in its `PARAM.SFO`, not GR33N. Its interface assets inspired an original procedural midnight-blue Xbox 360 theme; none were imported into the XEX. Details: [DEEPPLAY_PKG_ANALYSIS.md](DEEPPLAY_PKG_ANALYSIS.md).

A separate [TLS 1.3 / Microsoft device-code migration](TLS_ACCOUNT_INTEGRATION.md) now documents the MIT XboxTLS13 C++ source, consolidation into the main XEX, original licensing and current network blockers. It is **not** evidence of a successful login.

## Important findings

**GR33N lists the following dependency build order:**

1. `mbedTLS` — TLS and DTLS. Need a validated entropy source, real clock/certificate store and a networking backend.
2. `libSRTP` — secure real-time packet transport.
3. `usrsctp` — WebRTC data channels when required by the negotiated session.
4. `libpeer` — lightweight WebRTC peer implementation and ICE/signaling integration.
5. `Opus` — audio decoding with a fixed-point build suitable for limited hardware.

This is a **research candidate**, not an installed or working dependency set on Xbox 360. Before bringing it in, validate CPU/endianness/ABI/compiler support, networking primitives, build flags, memory footprint, codecs actually negotiated, copyright notices and license compatibility.

**XCloud4** describes Device Code login and token handling. Device Code is a standardized flow, but **client registration, scopes, entitlement and streaming API authorization still need to be established for our own client**. Do not hardcode someone else's client identifiers or embed Microsoft passwords.

**XCTV Player and xbox_launcher** rely on Android or modern Windows web browser technology. These are references for the UI only, not solutions to missing TLS/WebRTC on Xbox 360.

## Target architecture on Xbox 360

```text
Xbox 360 controller -> XInput -> session input encoder -> authorized media data channel
                                               |
Xbox Cloud Gaming <-> TCP/UDP <-> TLS / DTLS / SRTP / ICE <-> session controller
                                               |
                  RTP -> depacketizer -> codec decoder -> Xenos display
                  audio codec decoder -> Xbox audio output
```

### Technical checkpoints

- [ ] RGH hardware: verify UDP/TCP can reach external IPs, or demonstrate a better network backend through libxenon/XeLL.
- [ ] Port TLS/DTLS (likely mbedTLS) with proper certificate and peer verification.
- [ ] Confirm Microsoft OAuth flow and allowed API access for a new client. Device Code parsing alone is insufficient.
- [ ] Prototype a negotiated WebRTC session using a lightweight library if service requirements permit.
- [ ] Port libsrtp/usrsctp only if needed by the actual signaling/data-channel flow.
- [ ] Prove controller messages can be transmitted into an authorized session.
- [ ] Prove a **locally generated or permissively licensed** H.264 sample can be decoded **on the Xbox 360 hardware** before attempting cloud video.
- [ ] Add audio decoder/output, jitter buffering and sync.
- [ ] Validate memory budget, endianness, CPU load and 720p performance. No frame-rate promise until measured.

## Licenses and provenance

- **GR33N and XCloud4:** GPLv3. **Do not copy/port source** into this repository without deciding on a compatible distribution license and retaining required notices.
- **XStreamingDesktop:** MIT, with attribution/notice obligations if code is reused.
- **XCTV Player:** GPLv2, requiring a compatibility review before combining with other copyleft work.
- **xbox_launcher:** check the repository's actual `LICENCE.md` and dependency licenses before code reuse.
- Xbox SDK/XDK samples, font data and XPR resources are proprietary; generated files stay on the developer's machine and must not be committed or mirrored to GitHub.

The updated UI takes only **general visual inspiration** (dark cards, Xbox green, game tile and control-friendly navigation); no images, assets or code from the referenced projects were copied.

## Outstanding project blockers

**Network remains unproven on the Xbox 360 RGH.** The last RGH report was DNS: 1 resolved IP, but TCP port 443 to Xbox.com and 1.1.1.1 timed out with `10060`, source `8` (`TCP_SELECT`). This is not a TLS/OAuth error because the TCP connection never completed. The optional diagnostics screen retains this information while the home UI no longer blocks on the test.

**No streaming client exists yet.** A working HTTPS connection would only complete one of many required milestones. A Microsoft account login alone does not prove that the xCloud session protocols or video decoding will work.

See [ROADMAP.md](../ROADMAP.md), [PROJECT_STATUS.md](../PROJECT_STATUS.md) and [RGH_NETWORK.md](RGH_NETWORK.md).
