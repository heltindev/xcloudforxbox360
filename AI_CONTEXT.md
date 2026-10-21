# XCloud360 — read this first

## Active project

The repository root is the only active codebase. Sources are in `src/`, dependencies in `third_party/`, tests in `tests/`, and the project file is `XCloud360.vcxproj`. Do not edit or build the old `XCloud360-GitHub-Source` copy; it is historical.

Goal: native xCloud client on RGH/JTAG Xbox 360 with no PC relaying media. An external browser may be used to approve the Microsoft device code.

## Platform and constraints

- Xenon PowerPC, 32-bit, big-endian; XDK/MSVC 2010 and the compiler-compatible C++ subset.
- D3D9 and XInput are already used. XAudio2 libraries are linked, but audio playback is not integrated.
- Do not assume full C++11/14, hardware H.264 decoding, or DTLS 1.3 availability.
- XboxTLS provides HTTPS over TCP, not WebRTC or DTLS-SRTP peer functionality.
- Respect protocol-specific byte order: RTP is big-endian; current xCloud control message fields use little-endian. SDP is text.
- Never log tokens, passwords, sign-in codes, or complete authentication responses.

## Verified state and known limitations

Sign-in cache: refresh tokens are stored in protected form at `game:/XCloud360.authcache` with application/scope binding. A attempts resumption before requesting a code; START on Home clears it. Transient network or session failures preserve the cache. See `docs/LOGIN_CACHE.md`. Native console protection has not been validated.

**NET-10060-A1:** bounded IP acquisition wait within SYSAPP, HTTPS attempts up to four IPv4 addresses, and the X / NETWORK screen (Y initiates testing from Home). See `docs/NETWORK_10060.md`. Compiling does not prove TCP 10060 is fixed on hardware.

The XEX compiles. The code includes UI, DNS/TCP/TLS, Device Code, Xbox User/XSTS/xCloud login, in-memory credentials, and experimental session control. `XboxAccount::perform_session` creates, polls the queue, attempts to connect, then stops the test session. This is **NOT continuous playback**.

Control formatters and RTP/H.264 reassembly exist. Missing pieces include media negotiation, a functional ICE/DTLS/SRTP/SCTP peer, jitter/NACK handling, decoders, video/audio output, and continuous integration. `src/webrtc_xbox.h` is a declaration-only prototype without a matching implementation.

Simulated tests do not prove real authorization, service access, or operation on console. See `docs/VALIDATION_CURRENT.md` for this revision's binary and tests.

## Read only what is needed

1. This file.
2. `REFERENCE_BASE/AI_REFERENCE.md` for a short map and references.
3. `docs/XEX_GAPS.md` for streaming blockers and diagnosis.
4. Files relevant to the task only; use `SOURCE_MAP.json` for hashes and reading priorities instead of opening the entire research archive.

| Area | Key files |
| --- | --- |
| Entry point/UI | main.cpp, app.cpp, platform_xbox360.cpp |
| Sign-in | xbox_account.cpp, xbox_device_flow.cpp, xbox_live_exchange.cpp |
| Session | xcloud_session.cpp, xboxtls_backend.cpp, http_wire.cpp |
| Controls/bridge | xcloud_bridge.cpp, xcloud_input.cpp, xcloud_control.cpp |
| RTP/H.264 | rtp_packet.cpp, h264_rtp_receiver.cpp |

Paths in the table are relative to `src/`.

## Commands

```powershell
.\tests\run_host_tests.ps1
.\build_release.ps1
.\REFERENCE_BASE\refresh_context.ps1
.\package_github.ps1
```

Current output: `Release/XCloud360.xex`. Do not publish local configuration, SDKs, generated resources, reference binaries, or `obj/` files. The context script explicitly selects files and hashes rather than concatenating the entire codebase.

**AUTH-DIAG-A2 diagnostics:** X shows separate cache, Xbox User, XSTS, xCloud, and session results. Build and Retail checks passed; hardware layout/behavior remain unvalidated. Previous host suites were not repeated after the UI change. See `docs/CURRENT_DIAGNOSIS.md`.

## Paused development and continuation

The author paused development and permits forks to continue the author's contribution. `README.md` and `docs/BUILD_VS2010.md` document the handoff. `prepare_vs2010.ps1` creates example `auth_config.h`, fonts, and local ATG. `GITHUB-PAUSADO` is a publication snapshot, not another active codebase. SDK/XDK and proprietary tools are not distributed.
