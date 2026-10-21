# What is still missing for working xCloud

Local code review dated 2026-10-21. Building an `.xex`, running the UI, and playing an xCloud session are three different milestones.

## The executable already has an entry point

`src/main.cpp` uses `main()`, validates PowerPC/32-bit/big-endian requirements, and calls `App::initialize -> run -> shutdown`. `platform_xbox360.cpp` initializes D3D9, fonts, and XInput. There is no need to invent `XamMain` to replace this entry point.

The networking components initialize XNet/WSA themselves; do not add a fictitious `XNetInit` in main. Address acquisition time, socket context, and real errors must be measured on hardware.

## Libraries already linked

The Release `XCloud360.vcxproj` links AtgFramework, xapilib, d3d9, d3dx9, xgraphics, xboxkrnl, xnet, xonline, xaudio2, xact3, x3daudio, xmcore, vcomp, and xbdm, with corresponding Debug variants. ATG and the embedded font are prepared locally during build.

Linking `xaudio2.lib` does not implement a player. Do not add `vnet.lib` just because a guide mentions it; that library was not found in the examined local library set and is not an identified project dependency.

## Existing session code is experimental

`xcloud_session.cpp` implements restricted endpoints for POST play, GET state, POST connect, and DELETE of the returned session. It handles known states, polling limits, HTTP/TLS rejection, and cleanup.

`XboxAccount::perform_session` consumes the credential, attempts login renewal, requests a transfer token when needed, and tries to end the session. The flow stops at Provisioned and releases the remote machine. There is still no SDP/ICE exchange or continuously running media. Formats are experimental and require real validation.

## Concrete blockers

| Stage | Existing files / remaining work |
| --- | --- |
| Real networking | TCP probes and TLS backend exist. Locate console TCP 10060: address/link, DNS, connect, TLS, or HTTP reading |
| Authorization | Device Code, Xbox User, XSTS, login, refresh, and transfer code exist. Verify consent, permissions, and real responses |
| Signaling | Extend Session/backend with required endpoints, SDP/ICE responses, configuration, and keepalive; preserve allowlists and cleanup |
| WebRTC peer | `webrtc_xbox.h` contains declarations without a matching cpp. UDP sockets, ICE/STUN/TURN, retransmissions, and pair selection are missing |
| Media security | Integrate server-compatible DTLS, verified fingerprint, DTLS-SRTP exporter, SRTP/SRTCP, and replay protection |
| DataChannels | Integrate SCTP over DTLS and channel open/close handling; connect existing message formatters |
| Video | Order packets/recover loss, handle RTCP/NACK/PLI and jitter before receiver; choose/port a real H.264 decoder |
| Audio | Receive and reorder Opus, decode with PLC when appropriate, and feed XAudio2 SourceVoice with correct buffer lifetimes |
| Presentation | Upload decoded frames to D3D9 textures and synchronize with audio using clocks and timestamps |
| Continuous operation | Keep sessions alive during media, renew credentials, handle reconnection, cancellation, and resource cleanup |

An accessible raw H.264 hardware-decoding API has not been demonstrated in the installed XDK. D3D9 renders already-decoded images. Evaluate actually available APIs or a ported software decoder and measure its cost on Xenon.

## Dependencies that still need porting

GR33N uses mbedTLS, libSRTP, usrsctp, libpeer, and Opus; its scripts build for PSL1GHT. XDK ports require compatible build configuration, ABI, timers, entropy, sockets, threads, alignment, intrinsics, and byte order for PowerPC/MSVC 2010. Preserve licenses and pin revisions.

XboxTLS implements TLS for HTTPS, not interchangeable DTLS. Do not assume full C++11/14 support in MSVC 2010 or link PS3 code / `.a` libraries directly into the XDK project.

## Completion criteria

1. Real login code received and authorized on the console, without disclosing tokens.
2. Authorized session provisioned and termination confirmed.
3. ICE connected, DTLS fingerprint verified, and SRTP authenticated using real packets.
4. H.264 frames decoded, Opus audio played, and remote control accepted.
5. Continuous gameplay with bounded memory, measured latency, and cleanup on errors.

Host tests validate simulated responses. Building and Retail inspection verify the artifact. Neither replaces these criteria.

## Technical references

- RTP/RTCP and timestamps: https://www.rfc-editor.org/rfc/rfc3550
- DTLS-SRTP: https://www.rfc-editor.org/rfc/rfc5764
- WebRTC transports: https://www.rfc-editor.org/rfc/rfc8835
- DataChannels: https://www.rfc-editor.org/rfc/rfc8831
- MSVC compliance: https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance

Older reports under `docs/` and archived copies describe their own revisions; `AI_CONTEXT.md`, this diagnosis, and `VALIDATION_CURRENT.md` describe the current baseline.
