# Harissa-inspired Xbox 360 UI and Xbox Live HTTPS exchange

**Source changes:** 2026-10-21  
**Build/testing status for this source revision:** **NOT COMPILED, NOT TESTED IN XENIA, NOT TESTED ON RGH3**, as requested by the project owner. The existing binary in `Release/` is the **older compiled revision**.

## 1. Harissa64-inspired front end

The original Harissa64 project, held privately at `PASTACODIGOBASE/Harissa64-master`, is a Nintendo 64 emulator for modded Xbox 360 consoles, **not** an Xbox Cloud Gaming client. Its distinctive front end uses a two-pane game browser, highlighted selected row, game art, details, compact status pills and a fixed footer.

The new **independently implemented** native UI in `src/harissa_style_ui.cpp/.h` and `src/app.cpp` has:

- A left game-library pane with **Fortnite** as the only available entry and an **A** action to open a separate login screen.
- A right detail pane with an **original abstract orange skyline** (not Harissa64 box art, copyrighted Fortnite promotional artwork or extracted PS3 assets), metadata, status badges and a prominent controller action.
- A dark graphite background, muted white text and coral/orange highlights; the application's existing embedded smooth Xbox 360 font is reused.
- An authentication screen with the same two-pane layout, Microsoft server-provided verification URL/code and countdown, plus individual Xbox User and XSTS indicators.
- Existing **B** (games), **A** (retry), **X** (network), **Y** (offline tests) and **BACK** (exit) navigation.

The design borrows **visual ideas only**, not GPL source code or assets. Harissa64 is GPL-2.0 and GR33N is GPL-3.0; all their reference code remains in the private `PASTACODIGOBASE/` folder. Do not copy GPL implementation into this project without reviewing redistribution and compatibility obligations.

**Evidence:** a prior revision of this interface was compiled to Retail XEX and successfully rendered in Xenia (private image `PASTACODIGOBASE/ARQUIVOS_ANTIGOS/xenia_harissa_style_home_20261021.png`). **The subsequent Xbox HTTPS and offline-test code changes have not been compiled.**

## 2. What was learned from GR33N

The GR33N PS3 application is a genuine research reference for:

- Microsoft OAuth device code, an Xbox-specific login permission, Xbox User Token and service-dependent XSTS relying party.
- xCloud offering/service tokens, region discovery, queueing, session signaling and eventual WebRTC.
- H.264 RTP depacketization, a PS3-specific H.264 hardware decoder and Opus media handling.
- ICE, DTLS, SRTP, SCTP data channels, controller messaging and rendering.

PS3 APIs (`sysModuleLoad`, `vdec`, libpeer build scripts, PSL1GHT, RSX) **cannot be linked into an Xbox 360 XEX**. This revision implements native, independent Xbox 360 sources. It does not port the full PS3 media stack.

## 3. Real Xbox User / XSTS HTTPS code (unverified)

Files:
- `src/xbox_live_auth.cpp/.h` — scope checks and service-specific, bounded JSON request bodies. Xbox User Token is not the same as XSTS; an XSTS relying party for `http://gssv.xboxlive.com/` is distinct from the generic `http://xboxlive.com` relying party.
- `src/http_wire.cpp/.h` — inserts the required fixed `x-xbl-contract-version: 1` header into JSON POST requests without accepting untrusted header text.
- `src/xboxtls_backend.cpp/.h` — only accepts `user.auth.xboxlive.com/user/authenticate` and `xsts.auth.xboxlive.com/xsts/authorize` in the new JSON route. The existing verified TLS 1.3 handshake and hostname check are required. There is **no HTTP downgrade or certificate bypass**.
- `src/xbox_live_exchange.cpp/.h` — performs a two-request exchange in a background worker, validates HTTPS evidence and JSON token fields, wipes all token-bearing temporary buffers and exposes **only** bounded stage/HTTP/`XErr` diagnostics. For xCloud-specific XSTS it selects `http://gssv.xboxlive.com/` as the relying party.
- `src/xbox_device_flow.cpp/.h` — offers the OAuth access token **only to its own worker** when the token response explicitly granted an Xbox sign-in scope. Both `XboxLive.signin` and `xboxlive.signin` spellings are recognized as exact scope tokens; substring matches are rejected.
- `src/xbox_account.cpp/.h` — moves from Microsoft OAuth to the Xbox exchange only with confirmed scope and only while the login has not been cancelled. No tokens are returned to the UI or stored in files.
- `src/app.cpp` — shows Xbox User and XSTS approval separately, or a safe HTTP/`XErr` code and the first incomplete dependency.

The example `auth_config.example.h` keeps the Xbox scope opt-in setting **off by default**. The existing user's private `auth_config.h` was **not modified**. Enabling the setting requires an application legitimately registered and permitted to request Xbox Live authorization. If an API denies the scope or service token, the app reports that denial rather than attempting to bypass it.

**Important limitation:** `tls_trust.cpp` currently embeds a narrow CA set. The Xbox authentication hosts may chain to different CAs. A TLS failure in that case must be fixed by correctly maintaining verified trust anchors, not by disabling validation.

## 4. Added offline test scenarios

`src/protocol_selftest.cpp` now references:

- Existing cryptographic, DNS-independent protocol, JSON, OAuth, RTP/H.264 and input packet tests.
- A synthetic Xbox-compatible JSON POST test checking `Content-Type: application/json` and `x-xbl-contract-version: 1`.
- `xbox_live_exchange::offline_selftest()`: permission absent, valid two-stage simulated response, rejection of an unverified TLS reply, and a synthetic 401 with `XErr`.
- `xcloud_bridge::offline_selftest()`: synthetic secure-channel handshake, rejection of unconfirmed channels, consecutive gamepad/metadata sequence numbers after a failed send, keyframe request and H.264 IDR access-unit callback.

These tests **have been added to source but have not been executed** for this revision. The fake network results cannot demonstrate a real Microsoft/Xbox authorization or a Fortnite stream.

## 5. Still needed for an actual Fortnite session

1. Successfully build the updated Xbox XDK project and inspect compiler warnings/errors; repair them before deploying.
2. Run the offline tests in Xenia, then validate networking/TLS and the login on a **physical RGH3 Xbox 360**. The previous physical TCP error 10060 remains unresolved.
3. Confirm the user's own Microsoft app is authorized for Xbox sign-in and the two service endpoints return valid user and service XSTS tokens over fully trusted TLS.
4. Determine whether Xbox Cloud Gaming permits this particular client to obtain an **authorized** service token, cloud offering, catalog entry, session and negotiated signaling. A generic Xbox/XSTS success does not grant the right to use xCloud.
5. Implement ICE/STUN/TURN, DTLS/SRTP, SCTP/WebRTC, video jitter handling, actual H.264 decoding on Xenon/Xenos or feasible optimized software, Opus audio, synchronization, and low-latency controller input.
6. Test end-to-end on the RGH3 console with actual gameplay and performance measurement.

The retail, unmodified Xbox 360 cannot normally execute the unsigned homebrew XEX, and is not an officially supported Xbox Cloud Gaming console.

## Current honest status

| Milestone | Current revision |
| --- | --- |
| Harissa-inspired homepage | Written, with previous UI revision visually validated in Xenia |
| Microsoft Device Code | Previously verified in Xenia; source unchanged in essential login behavior |
| Xbox Live / XSTS HTTP exchange | **Source implemented, NOT compiled or tested against live Xbox service** |
| Cloud session / WebRTC / audio/video decoder | **Not implemented** |
| Actual Fortnite gameplay | **Not working** |
| Xbox 360 RGH3 physical test | **Not performed for this build** |

No ZIP created. The current version is source code, not a tested or released binary.
