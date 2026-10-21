# Preparação da pré-release

## Campos do GitHub

- Tag sugerida: `v0.10.0-alpha.2`.
- Release title: **XCloud360 v0.10 Alpha 2 — Xbox 360 PowerPC Homebrew**.
- Previous tag: `v0.10.0-alpha.1`, conforme a release anterior informada.
- Release label: **None**.
- Pre-release: **marcar**.
- Tag e publicação não foram criadas por esta edição.

## Release description — copiar a partir daqui

# XCloud360 v0.10 Alpha 2 — Xbox 360 PowerPC Homebrew

**Experimental Xbox Cloud Gaming client development for Xbox 360 RGH/RGH 3.**

This update advances native authentication, in-memory credential management and HTTPS response handling toward a console-direct client without an intermediary PC.

> **ALPHA RELEASE — NOT PLAYABLE**
>
> Authentication code is implemented, but real Microsoft/Xbox/xCloud authorization has not been validated in this revision. Cloud streaming and Fortnite gameplay are not available. The PowerPC build and offline tests passed; this revision still requires execution on Xenia and real Xbox 360 hardware.

## What's New

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

Credential storage is implemented; a game-session consumer and automatic renewal are still pending.

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
- [ ] Create a game session, handle its queue, connect and terminate it.
- [ ] Connect credentials to session requests and implement renewal.
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
4A51BAFC00AA898709314051B659B9F897562DDF6E3B27F438602F23E5B85E36
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

## Fim da descrição — orientação para anexar

Em **Attach binaries**, selecione:
`C:\Users\hhhel.DESKTOP-3QIFGGI\Documents\XCLOUDFORXBOX360\Release\XCloud360.xex`.

A pasta `XCloud360-GitHub-Source` contém os fontes. O binário está apenas na pasta principal. A documentação desta edição foi ajustada; não houve nova compilação.
