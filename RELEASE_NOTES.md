> Development paused by the author on 2026-10-21. Forks are welcome. xCloud is not playable yet. See README.md and docs/VALIDATION_HANDOFF.md; the earlier release notes follow.

# Pre-release preparation

## GitHub release fields

- Suggested tag: `v0.10.0-alpha.2`.
- Release title: **XCloud360 v0.10 Alpha 2 — Xbox 360 PowerPC Homebrew**.
- Previous tag: `v0.10.0-alpha.1`, as reported in the preceding release.
- Release label: **None**.
- Pre-release: **checked**.
- No tag or release was published by this edit.

## Release description — copy from here

# XCloud360 v0.10 Alpha 2 — Xbox 360 PowerPC Homebrew

**Experimental Xbox Cloud Gaming client development for Xbox 360 RGH/RGH 3.**

This update advances native authentication, in-memory credential management and HTTPS response handling toward a console-direct client without an intermediary PC.

> **ALPHA RELEASE — NOT PLAYABLE**
>
> Authentication code is implemented, but real Microsoft/Xbox/xCloud authorization has not been validated in this revision. Cloud streaming and Fortnite gameplay are not available. The PowerPC build and offline tests passed; this revision still requires execution on Xenia and real Xbox 360 hardware.

The project root is now the only active source tree. Experimental session creation, queue handling, connect, renewal and cleanup code is included; the session test ends without media playback. See docs/XEX_GAPS.md and docs/VALIDATION_CURRENT.md.

## What's New

### Protected login cache

- Saves the Microsoft refresh token using console protection; resumes before requesting another device code.
- Preserves the saved account on transient network errors or a failed cloud session.
- START on home removes the local saved login.
- Native console persistence is implemented but not yet runtime-validated; host DPAPI and simulated resume tests passed.


### Network diagnostics NET-10060-A1

- Bounded wait for local address acquisition on SYSAPP.
- Tries multiple returned IPv4 addresses within a total TCP wait budget.
- Controller-accessible diagnostics: X opens the screen; Y runs a network-only test from home.
- Built with Visual Studio 2010; readiness regression tests and Retail inspection passed.
- The reported TCP 10060 on hardware is still pending validation.


### Native Interface

- Simplified dark interface with green accents and controller navigation.
- Microsoft device-code login screen with verification address, code and remaining time.
- Non-secret HTTP, TLS and Xbox error diagnostics.

### Experimental Authentication

- Implemented Microsoft Device Code flow over verified HTTPS.
- Added Xbox User, service-specific XSTS and xCloud service login requests.
- Fixed parsing of nested Xbox authentication responses.
- Validated cloud token lifetime and returned HTTPS Xbox regions.

These are implementation milestones, not confirmation of successful service authorization with a real account.

### In-Memory Credentials

- Retains the cloud token and selected region only in the authentication worker.
- Prefers a valid default region when supplied.
- Expires credentials using a monotonic clock, with a maximum retention of 24 hours.
- Clears credentials on cancellation, expiry or failed replacement.
- Checks cancellation between authentication requests.
- Prevents duplicate login attempts while authenticated.

Credential storage is implemented; experimental session requests and renewal now have code; live validation and continuous playback remain pending.

### HTTPS Reliability

- Completes Content-Length and chunked responses without waiting for the server to close the connection.
- Rejects unsupported transfer encodings and ambiguous HTTP headers.
- Adds regression coverage for fragmented responses, cancellation, expiry and clock rollover.

This addresses HTTP response handling after TLS connects. It does not establish that the reported initial TCP timeout has been fixed.

## Validation

| Test | Result |
| --- | --- |
| Offline authentication, credential and HTTP regression tests | Passed |
| Xbox 360 PowerPC Release compilation | Passed |
| Retail XEX inspection | Passed |
| Real Microsoft consent and Xbox/XSTS/xCloud authorization | Not validated in this revision |
| Execution on Xenia | Not performed for this revision |
| Execution on physical Xbox 360 | Not performed for this revision |
| Cloud video, audio and gameplay | Not implemented |

## Development Checklist

- [x] Native green interface and controller navigation.
- [x] Experimental verified-HTTPS authentication code.
- [x] Nested JSON response parsing and region validation.
- [x] In-memory cloud credentials with expiry and cleanup.
- [x] HTTP response completion fixes.
- [x] Offline regression tests and PowerPC compilation.
- [ ] Validate authentication with a real account.
- [ ] Investigate the reported TCP 10060 timeout on Xbox 360.
- [x] Implement experimental creation, queue handling, connection and termination.
- [ ] Validate the lifecycle with the service and keep the session running during media.
- [x] Implement credential use and renewal in the session test.
- [ ] Validate renewal against the real service.
- [ ] Integrate WebRTC signaling, ICE, DTLS/SRTP and data channels.
- [ ] Integrate H.264 video decoding and rendering.
- [ ] Integrate Opus audio decoding and synchronization.
- [ ] Connect controller input to the real remote channel.
- [ ] Test memory, performance, latency, reconnection and stability on hardware.

Selecting Fortnite starts diagnostics and authentication, not a playable game.

See [ROADMAP.md](https://github.com/heltindev/xcloudforxbox360/blob/main/ROADMAP.md) for the full checklist.

## Building and Experimental Binary

The project requires a compatible Xbox 360 development environment, an authorized Xbox 360 SDK/XDK installation and the appropriate PowerPC build tools.

Build output: `Release/XCloud360.xex`.

The locally compiled XEX has this SHA-256:

```text
A41E62588D4E7BE41E3BDB18F77B626131886E626CE68195A8413DAF44D408B4
```

Compilation and Retail inspection do not establish runtime compatibility or playable streaming.

## Acknowledgements and Research References

Thanks to the projects used as architectural and protocol research references:

- [XStreamingDesktop](https://github.com/Geocld/XStreamingDesktop)
- [XCTV Player](https://github.com/kjbaker-uk/xctvplayer)
- [xbox_launcher](https://github.com/LuanRoger/xbox_launcher)
- [XCloud4](https://github.com/asp11edgr/XCloud4)
- [GR33N](https://github.com/actuallyniaxx/GR33N)

XCloud360 is an independent homebrew research project and is not affiliated with Microsoft, Xbox or Epic Games.

**Help keep the Xbox 360 alive. 🎮**

## End of description — attaching the artifact

Use the current local XEX at Release/XCloud360.xex. All active project sources are in the XCLOUDFORXBOX360 root. The hash above corresponds to the build verified in this layout, without console validation.
