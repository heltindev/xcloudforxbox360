# Fortnite streaming target — Xbox 360 PowerPC

This document describes a **desired architecture**, not a working application. There is no native Fortnite executable for the Xbox 360 in this project. The goal is to render a legitimately authorized **Xbox Cloud Gaming** session on the original hardware.

## Required pipeline

```text
             Xbox Cloud Gaming service (remote servers)
                    |                   ^
              video/audio           controller input
                    |                   |
                  secure real-time transport
                    |                   |
               Xbox 360 Xenon PowerPC client (.xex)
                    |                   |
                  decoder          XInput data channel
                    |
                  Xenos GPU / audio -> TV
```

## Missing milestones

1. Reliable external TCP **and** UDP on the physical RGH.
2. Modern TLS/DTLS with certificate and hostname verification.
3. Authorized Microsoft Device Code OAuth flow and legitimate xCloud session permissions.
4. Session discovery and catalog/entitlement for Fortnite in the account/region.
5. Signaling, ICE/STUN/TURN, secure RTP and real-time controller transport negotiated with the current service.
6. Compatible video decoder and Xbox 360 Xenos rendering path for a permitted/negotiated codec.
7. Audio codec decoder, Xbox audio output, synchronized buffers and jitter recovery.
8. Safe token handling, cancellation, reconnection, memory and latency measurements.
9. A real-console demonstration with audio, video and control, without intermediary PC.

### Experimental source already present

- `http_wire`: limited HTTP/1.1 framing, no network/TLS integration.
- `json_fields` and `oauth_device`: local parsing of Device Code metadata; no real sign-in.
- `rtp_packet`: header-only RTP parser; not DTLS/SRTP/WebRTC.
- `h264_nal`: H.264 Annex B scanner; **not** a decoder.
- `controller_input`: XInput snapshots; no cloud input channel.

## Research projects

- [GR33N (PS3)](https://github.com/actuallyniaxx/GR33N): PowerPC/endianness, native TLS/DTLS, SRTP, usrsctp, libpeer and fixed-point Opus.
- [XCloud4 (PS4)](https://github.com/asp11edgr/XCloud4): native Device Code sign-in and session UX.
- [XStreamingDesktop](https://github.com/Geocld/XStreamingDesktop): modern xCloud client and controller UX.
- [XCTV Player (Google TV)](https://github.com/kjbaker-uk/xctvplayer): fullscreen controller-first TV interface.
- [xbox_launcher](https://github.com/LuanRoger/xbox_launcher): Xbox Series-inspired dashboard design.

See [REFERENCES.md](REFERENCES.md) for the technology mapping and license restrictions and [ROADMAP.md](../ROADMAP.md) for completion criteria.

**These references are independent projects. Their claims and platform-specific implementations are not proof of support on the Xbox 360.**
