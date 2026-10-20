# GR33N (PS3) to XCloud360 (Xbox 360 XEX): Native Porting Notes

**Research snapshot:** [actuallyniaxx/GR33N](https://github.com/actuallyniaxx/GR33N), commit [bb71f0c](https://github.com/actuallyniaxx/GR33N/commit/bb71f0cab3a5edd42492a4d2baef69e10d7a9963) (October 13, 2026).

**Target:** native `Release/XCloud360.xex` for **Xbox 360 Xenon PowerPC (RGH/RGH 3)**, launched via Aurora / XeXMenu, **without a PC acting as a streaming proxy**.

> **Status:** The Xbox 360 client is still **NOT able to sign in to Xbox Cloud Gaming or play Fortnite**. This work ports selected data-format and architectural concepts, **not the complete GR33N runtime**. Source changes await compilation and physical Xbox testing by the project owner.

## What was inspected in GR33N

- `source/xcmsg.c` and `include/xcmsg.h`: control-channel messages, gamepad reports and **explicit little-endian serialization on big-endian PowerPC**.
- `source/input.c`: controller state and analog triggers, adapted to Xbox 360 XInput rather than PS3 pad APIs.
- `source/vjitter.c` and `include/vjitter.h`: H.264 RTP frame assembly, packet-loss recovery, IDR gating, jitter and RTCP NACK.
- `source/webrtc.c`: channel setup, handshake acknowledgements, input cadence and callback threading.
- `source/net_tls.c`: mbedTLS socket integration and certificate handling on the PS3.
- `source/auth.c` and `source/session.c`: device-code authentication, cloud-service authorization and session lifecycle.
- `source/decoder.c`, `source/aud.c` and `source/video.c`: PS3-native video/audio decoding and rendering.

## What has now been rewritten for Xbox 360 C++ / XDK

| Xbox 360 file | Native implementation | Verified? |
| --- | --- | --- |
| `src/xcloud_input.cpp` | Converts **XInput** buttons, two analog triggers and four thumb axes into fixed-length cloud-input packets, with explicit little-endian serialization | Source + offline tests written; **uncompiled on RGH** |
| `src/xcloud_control.cpp` | Bounded JSON formatters for handshake, gamepad connect/disconnect, resolution, client video capabilities and keyframe requests; parses `HandshakeAck` by type | Source + offline tests written; **uncompiled on RGH** |
| `src/h264_rtp_receiver.cpp` | Parses **decrypted RTP**, reconstructs **Single-NAL, STAP-A, FU-A** into H.264 Annex B access units; resets and requires IDR after packet loss | Source + offline tests written; **uncompiled on RGH** |
| `src/xcloud_bridge.cpp` | Native glue between an eventual secure WebRTC peer, controller input reports, control messages and Annex B decoder callbacks; refuses to send without session-ready callbacks | **Not integrated with a real WebRTC peer** |
| `src/protocol_selftest.cpp` | Fixed offline packet examples exercising input endianness, handshake messages, FU-A, STAP-A, loss/IDR gating | Owner must compile and confirm **PARSERS OFFLINE: OK** |
| `XCloud360.vcxproj` | Includes all new `.cpp`/`.h` files under the Xbox 360 target | Project XML only; compilation not run |

**The code is written for Xenon PowerPC, not PS3 or Windows.** Its wire serializers avoid `memcpy` of host-order integer structures.

### Data-channel convention studied

GR33N documents channels `control=0`, `input=2`, `message=4`, `chat=6` on its peer implementation. This version represents that convention in `xcloud_bridge.h`. **Channel availability, security and authorization must come from a real negotiated session** and may differ with protocol versions.

The standalone bridge does not invent registration IDs, Microsoft passwords, tokens or the reference project's hard-coded client identification values.

### Limitations of the video receiver

The initial Xbox 360 implementation accepts in-order RTP packets and **drops broken/out-of-order frames**, requiring a complete IDR afterward. Unlike GR33N's larger `vjitter` implementation, it **does not reorder or buffer 200 ms of RTP packets, generate RTCP NACK/PLI, or decode H.264**. Those are future steps. The 512 KiB access-unit buffer is fixed-size and must be fed only with **authenticated and decrypted** media.

## Why the rest of GR33N cannot be recompiled as a XEX

| PS3 dependency/runtime | Needed Xbox 360 replacement |
| --- | --- |
| PSL1GHT, PS3 `sysThread`/mutex, BSD socket wrapper | Xbox 360 XDK threading/networking and verified RGH TCP/UDP backend |
| mbedTLS and PS3 entropy/clock/CA-store adaptation | Xenon PowerPC port, secure randomness, clock, root certs and **verified TLS/DTLS** |
| libpeer | Xbox-native, license-compatible WebRTC build, ICE/STUN/TURN and signaling |
| libSRTP | Port of SRTP cipher, replay protection and authenticated media receive |
| usrsctp | SCTP/data channels if the negotiated xCloud protocol requires them |
| Cell `cellVdec` video decoder | Xbox 360-accessible H.264 decoding hardware/API or realistically benchmarked software decoder |
| PS3 RSX textures/display | Xbox 360 Xenos GPU/D3D9 surfaces, video textures and color conversion |
| PS3 audio/Opus | Xenon-compatible Opus decoder plus Xbox 360 audio output and synchronization |
| PS3 `ioPad` | Xbox 360 `XInputGetState` (partially mapped in the new code) |

**Important network blocker:** real RGH testing previously resolved **one Xbox.com IP address**, then timed out on TCP to both `www.xbox.com:443` and `1.1.1.1:443` with **10060 / source 8 (TCP_SELECT)**. Until the platform can exchange traffic, a full session is impossible. Do not disable TLS security or claim authentication works to work around that network problem.

## The remaining sequence required for Fortnite

1. **Prove real Xbox 360 network connectivity.** Validate TCP and bidirectional UDP, investigate XNet Retail restrictions, or prove a separate libxenon/lwIP path.
2. **Port verified TLS/DTLS.** Integrate a crypto library with correct CA validation, clock, SNI and certificate hostname checking.
3. **Establish legal xCloud API authorization.** Register an appropriate Microsoft client and validate applicable service permissions. Standard OAuth Device Code alone does not grant arbitrary streaming API access.
4. **Implement real device-code login** using a verified HTTPS backend, including token-polling and secret-free logging.
5. **Implement session discovery/create/heartbeat** using current, authorized service APIs and negotiated signaling.
6. **Port a secure WebRTC peer** with ICE, DTLS, SRTP and data channels. Only then connect its callbacks to `xcloud_bridge`.
7. **Implement a real media jitter buffer/NACK/PLI**, using GR33N's design as a reference, and then a real H.264 decoder on Xbox 360.
8. **Implement Opus or the negotiated audio codec**, synchronized audio output, input feedback and latency management.
9. **Verify Fortnite gameplay on the physical RGH** at measurable resolution/frame rate/latency.

## Licensing and provenance

**GR33N is GPL-3.0**. The new Xbox 360 files above are independently authored implementations of packet/message format information and architectural ideas **without pasting upstream functions or including PS3 libraries**. If any GR33N source code or patched GPL dependencies are later imported or directly translated, the combined project must comply with the relevant GPL distribution terms and retain upstream copyright/license notices. Before releasing any derived work, review compatibility with the proprietary Xbox 360 SDK and distribution requirements.

The GR33N source was cloned **temporarily for reference only**, not included in XCloud360's published source tree. No binaries, tokens, firmware or proprietary SDK assets have been copied.

Related files: [ROADMAP.md](../ROADMAP.md), [STATUS_DO_PROJETO.md](../STATUS_DO_PROJETO.md), [REFERENCES.md](REFERENCES.md), [RGH_NETWORK.md](RGH_NETWORK.md).
