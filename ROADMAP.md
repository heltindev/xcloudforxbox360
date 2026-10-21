# XCloud360 — Complete development roadmap

**Platform:** real Xbox 360 RGH/RGH 3, native Xenon PowerPC, no intermediary PC.  
**Goal:** create an authorized Xbox Cloud Gaming session, stream Fortnite video/audio to the Xbox 360, and send the Xbox 360 controller input back to that session.  
**Last confirmed real-console network result:** Xbox DNS resolved 1 IPv4 address, but TCP connections to `www.xbox.com:443` and `1.1.1.1:443` both timed out with error `10060`, source `8` (`TCP_SELECT`).  
**Important:** this project is **NOT a functional xCloud client**. Source implementations are not equivalent to successful real-console tests.

Checklists have three distinct meanings:
- **Implemented source:** a source file exists, but it may not have been compiled or tested.
- **Validated on RGH:** the required feature demonstrably ran on physical hardware.
- **Blocked / pending:** missing functionality, unknown service access, or failed testing.

## Latest build and Xbox Live blocker (2026-10-21)

- [x] Independently redesigned Harissa-inspired Fortnite homepage and login in emerald green, with sharper contrast, 1080p-first native rendering and 720p/480p fallback.
- [x] Generated a private native **Segoe UI 42pt** atlas from the user's installed Windows/XDK tooling; embedded the XPR in the XEX with pixel-density-aware scaling. Kept `SegoeUI_24` as a fallback. Public source includes only `generate_hd_font.ps1`, no font assets.
- [x] Compiled the entire Xbox User/XSTS HTTPS stage and native protocol selftests with Xbox 360 XDK; XexTool confirmed compressed Retail XEX. Final source build passed without reported errors.
- [x] Xenia confirmed the green homepage, separate Microsoft Device Code screen, and genuine HTTPS device-code response; private screenshots do not go into GitHub.
- [x] Verified using the developer PC's Windows certificate store that both public Xbox auth hosts currently chain to **DigiCert Global Root G2**, already trusted by the native source. Hardware TLS handshake is a separate untested step.
- [x] Clarified Xbox login blocking condition: `xboxlive.signin` must be genuinely granted to the owner's registered app. The default private configuration remains unchanged and Xbox service HTTP calls are not falsely counted as completed.
- [ ] Validate actual Xbox User Token and service-specific XSTS responses with an authorized Xbox scope and the owner's explicit login; this did **not** occur in the Xenia smoke test.
- [ ] Develop authorized xCloud session creation, queue/signaling, WebRTC/ICE/DTLS/SRTP/SCTP, H.264/Opus decode, audio sync and controller return. Native Fortnite is **not running**.
- [ ] Test network and streaming on a real **RGH/RGH3 Xbox 360**. Last real-console TCP error 10060 is unresolved; Xenia success does not prove hardware compatibility.

For a grounded, precise test record see [Green HD and Xbox authorization report](docs/GREEN_HD_XBOX_2026-10-21.md).

## Previously uncompiled source milestone (subsequently built in Xenia)

- [x] Inspected `PASTACODIGOBASE/Harissa64-master` and adapted its two-pane browser **visually** to independent Xbox 360 D3D source in `src/harissa_style_ui.cpp/.h`. Earlier design compiled/booted in Xenia; no Harissa GPL code, font or art was copied.
- [x] Studied GR33N PS3 architecture for OAuth scope, Xbox User Token, service-specific XSTS, cloud session, WebRTC/ICE, H.264 and Opus separation. All PS3 code and packages remain **private reference only**.
- [x] Added native Xbox User/XSTS HTTPS exchanges, exact hostname/path allowlisting, contract-version header, scope gate, service-specific XSTS request, transient token handling and safe error state to source.
- [x] Added offline-test cases for an unverified TLS response, Xbox 401 XErr, synthetic service-token grant, gamepad metadata/packet sequence and H.264 depacketization across an in-memory bridge.
- [x] **COMPILED IN LATER UPDATE:** the owner subsequently requested compilation, and the new PowerPC source was built and XexTool-verified (see latest section).
- [x] **EXECUTED IN LATER UPDATE:** native startup offline tests ran before Xenia reached Microsoft's genuine device-code response. Test data is synthetic, not a game session.
- [ ] Obtain Microsoft permission for an owned `xboxlive.signin` public app and obtain valid Xbox User/XSTS tokens with TLS host validation; this has **never been confirmed**.
- [ ] Confirm the certificate trust set covers the Xbox authentication hosts, without disabling TLS verification.
- [ ] Validate direct DNS/TCP and authentication on an actual Xbox 360 RGH3 (previous hardware error 10060 still unresolved).
- [ ] Build a **permitted** xCloud service/session negotiation, WebRTC stack, authenticated SRTP, H.264/Opus decoding, A/V sync and controller return path. Current XEX is **not a playable cloud client**.

Documentation: [Harissa-inspired UI and Xbox HTTPS integration](docs/HARISSA_UI_XBOX_HTTPS_2026-10-21.md).

## Previous source milestone: distinguish Xbox permission from Microsoft OAuth (2026-10-21)

- [x] Confirmed Microsoft account device-code OAuth success is distinct from Xbox Live/XSTS and xCloud session authorization. The earlier `NOT IMPLEMENTED` message was a missing feature, not a crashing executable.
- [x] Integrated `xbox_live_auth.cpp/.h` into the native Xbox XDK project. Offline builders for Xbox User Token and generic XSTS request JSON, plus exact-scope checks, fail closed on invalid characters.
- [x] Verified the application **by default requests only basic Microsoft identity scopes**, not `XboxLive.signin`. Made Xbox Live permission request optional in the public configuration example; disabled by default and requiring an owned permissioned application registration.
- [x] Parse the explicitly returned OAuth `scope` in memory and only mark `XboxLive.signin` verified when actually present. Scope metadata is erased after the worker exits; no tokens are stored or logged.
- [x] Display the first actionable Xbox blocker after sign-in (typically `XBOX LIVE SCOPE NOT REQUESTED`) and **Fortnite streaming 0%**, without claiming that having a Microsoft account token starts xCloud.
- [x] Rebuilt the PowerPC compressed Retail XEX and boot-tested the one-game homepage and Microsoft device-code request under Xenia.
- [ ] Obtain permission to request Xbox scopes from a properly registered Microsoft public client; verify a real token's `scope` field.
- [ ] Implement actual certificate-verified HTTPS requests to Xbox User and XSTS endpoints; the offline JSON builders do not authenticate to Xbox services.
- [ ] Preserve authorized tokens safely and only for their actual intended service; the current OAuth flow **erases its token on worker exit**.
- [ ] Determine whether Microsoft allows an unofficial xCloud client to create an authorized cloud session; do not invent a session or claim streaming when permission is unavailable.
- [ ] Implement real WebRTC signaling/transport and H.264/audio decoding and gamepad return. Measure on physical RGH3. The stock retail Xbox 360 cannot normally execute unsigned homebrew XEX.

See [Xbox service-auth preflight report](docs/XBOX_AUTH_SCOPE_PREFLIGHT_2026-10-21.md).

## Current milestone: Fortnite homepage and Microsoft login screen (2026-10-20)

- [x] Added a **separate game-selection homepage**, with Fortnite as the sole available card. A transitions to a dedicated device-code login page; B returns to games.
- [x] The login page always uses the **actual `verification_uri` paired with the real Microsoft code**. The user-requested legacy `login.live.com/oauth20_remoteconnect.srf` link is labeled as a separate Live flow, **not forced onto an incompatible Entra v2 code**.
- [x] Added expiration, polling-attempt count, next-poll countdown, and visible `authorization_pending` / `slow_down` state.
- [x] Corrected the OAuth polling interval to begin **after** the HTTPS response; offline mock regression tests exercise `authorization_pending`, `slow_down` and success.
- [x] Compiled the native PowerPC XEX and verified it boots into the new homepage in Xenia.
- [ ] Confirm the **A-button transition to the separate login page** and complete the user's Microsoft approval (Xenia and real RGH3 are separate targets).
- [ ] Add an independently authorized **Live OAuth device-flow client**, using a properly registered legacy application ID, only if the legacy remoteconnect endpoint is strictly required. **Never borrow another application's client ID or cross-use v2 user codes.**
- [ ] Implement Xbox authorization/XSTS, xCloud session creation and negotiated WebRTC media before any claim of Fortnite playback.

See [homepage/login implementation and tests](docs/HOMEPAGE_MICROSOFT_LOGIN_2026-10-20.md).

## Current milestone: one-button Fortnite setup (2026-10-20)

The previous GR33N/DeepPlay four-tab UI in historical phases below is **superseded** by a one-entry Fortnite **text terminal**. A URL is shown for reference; this is **not a browser or game-streaming client**.

- [x] Replaced the terminal entrypoint with a Fortnite selector at `https://play.xbox.com/products/BT5P2X999VH2/fortnite`; one **A** button starts offline protocol tests, async DNS/TCP, then an independently verified Microsoft HTTPS Device Code request.
- [x] Added sequential numbered log steps and a percentage bar. Percentages are observable client setup only, capped at 70% for confirmed Microsoft device flow; xCloud play remains **blocked**, not fake 100%.
- [x] Fixed TLS 1.3 PSK-less early-secret HKDF input per RFC 8446. The native Xenia test received a **real device-code response** from the Microsoft server over TLS.
- [x] Added SHA-384/RSA PKCS#1 certificate support and cryptographic positive/negative unit tests; PowerPC cryptographic startup test passes in Xenia.
- [x] Updated the OAuth flow to rebase expiry after the HTTPS device-code response and hold **30 seconds before first token polling**; subsequent requests honor Microsoft's intervals, backoff and server-side expiry.
- [x] Captured a real Xenia screenshot of the redesigned Fortnite menu; offline tests show **PASS**. Compiled/released Retail PowerPC XEX successfully on the developer's PC.
- [ ] **OWNER XENIA INPUT TEST:** press **A** on a real gamepad, confirm steps 1–5 and read the real Microsoft code/expiry panel. Do not share the code, tokens or credentials.
- [ ] **OWNER RGH HARDWARE TEST:** run the same build on a physical RGH3. Earlier real RGH TCP 10060 is still unresolved; Xenia success does not prove RGH networking.
- [ ] Confirm user completes Microsoft OAuth token approval with a properly registered, permitted app ID; prior Xenia token request returned HTTP 400 and no authorization was confirmed.
- [ ] Implement authorized Xbox Live/XSTS, Xbox Cloud Gaming session launch, WebRTC/ICE/DTLS/SRTP, audio/video codecs, and controller input before asserting Fortnite is playable.
- [ ] Evaluate whether a full native browser is technically feasible; the current XEX has **no HTML5/WebView**. A direct `play.xbox.com` URL cannot boot a game by itself.

See [one-button Fortnite notes](docs/FORTNITE_ONE_BUTTON_2026-10-20.md) and [current status](STATUS_DO_PROJETO.md).

## Terminal-only native XEX — compiled and smoke-tested (2026-10-20)

- [x] Backed up the previous GR33N/DeepPlay dashboard source and project configuration under private `PASTACODIGOBASE/ARQUIVOS_ANTIGOS/DASHBOARD_GR33N_20261020/`.
- [x] Replaced `src/app.cpp` and `src/app.h` with a minimal **text terminal** using the Xbox 360's native Direct3D/XInput. No game grid, PS3 UI, WebView or HTML browser.
- [x] Fixed the duplicate object name collision for XboxTLS13's `platform_xbox360.cpp` and corrected the XDK network cleanup API.
- [x] Successfully compiled, linked and converted the current XEX to compressed Retail format. Confirmed SHA-256 and actual native Xenia launch.
- [x] Confirmed automatic offline protocol checks and regional JSON tests show **PASS** in Xenia.
- [x] Ran network and account diagnostics in Xenia: numeric IP 1.1.1.1 TCP 443/80 connected; Xbox.com 443 returned **10060**; Microsoft login failed at DNS before HTTPS, with no account authorization.
- [ ] **TEST ON REAL RGH3**: launch the new executable, rerun `Y`, `X` (network) and `A` (login) and record exact stages/codes. Emulator TCP behavior is not real-console validation.
- [ ] Decide whether a dedicated Xbox-native xCloud protocol client or a major native browser/HTML5/WebRTC engine port is viable. **There is no available web browser in this XEX.**
- [ ] Authorize xCloud sessions, implement DTLS/SRTP/WebRTC, H.264 decode, audio playback and controller return path. Fortnite is not currently playable.

Full evidence: [TERMINAL_TEST_RESULTS_2026-10-20.md](docs/TERMINAL_TEST_RESULTS_2026-10-20.md).

## Project root cleanup (2026-10-20)

- [x] Archived stale `dist/` separately from the active `Release/`, and moved the XeLL/libxenon proof-of-concept to private `PASTACODIGOBASE/XBOX360_EXPERIMENTOS/`.
- [x] Removed about **63.8 MiB** of regenerable IDE/build caches, including `bin/` and old intermediate `obj/` subfolders.
- [x] Preserved native XEX build prerequisites: font input, generated embedded-font C++, required `AtgFramework.lib`, XexTool, and the entire active `Release/`.
- [x] Updated the GitHub source-mirror script to exclude the archived XeLL experiment, checking hashes before deleting the obsolete copy.
- [ ] Compile and verify the simplified project layout on physical RGH; the TLS/account code has not yet been validated.
- Full inventory and archive paths: [ROOT_CLEANUP_2026-10-20.md](docs/ROOT_CLEANUP_2026-10-20.md).

## Source tree consolidation (2026-10-20)

- [x] Unified the former `XBOXTERMINAL/` standalone TLS experiment into the root Xbox 360 XEX sources, with MIT license and HTTPS/account adapter preserved.
- [x] Centralized all **locally available** PS3 reference trees, DeepPlay research extracts and the PKG inspector under private `PASTACODIGOBASE/`. No local PS4 checkout existed.
- [x] Kept the real Xbox 360 sources (`src/`, `third_party/`) separate from the PS3 research-only material.
- [x] Updated GitHub source mirror and packaging/sync guidance without copying proprietary PS3 resources, Xbox SDK assets or account configuration.
- [ ] Compile, link and run the unified TLS/account XEX on real RGH3. Existing error `10060` remains unverified after these changes.

## Phase 0 — Existing code / previous milestones

- [x] Native Xbox 360 C++ project targeting PowerPC via Xbox 360 XDK.
- [x] Older builds produced Xbox 360 Retail `.xex` files verified by XexTool.
- [x] A Direct3D-based UI and navigation with a real Xbox 360 controller.
- [x] Xbox XNet DNS resolved **one** address during a physical RGH test.
- [x] Socket diagnostics recorded TCP timeouts `10060`/source `8` on physical hardware.
- [x] Optional diagnostic screen; the home screen now shows project milestones and a Fortnite availability panel.
- [ ] **Compile and test the latest changes on a real Xbox 360**. The current updates were not compiled by the assistant.
- [ ] Verify the internal smooth XPR font loads from the XEX; bitmap fallback must be readable if it does not.
- [ ] Verify parser self-tests show `OK`, controller X/Y navigation works, and B/BACK exit cleanly.

## Phase 1 — Real network transport, first blocker

- [ ] Confirm IPv4, gateway, DHCP, DNS and console time on the RGH.
- [ ] Confirm network permissions and startup flags of the **Retail XNet** libraries.
- [ ] Inspect error `10060` and source `8` on **both** Xbox.com:443 and 1.1.1.1:443.
- [ ] Compare `1.1.1.1:80` against TCP 443; determine whether an entire port range or external TCP is blocked.
- [ ] Prove outbound **and inbound** UDP using a known, authorized endpoint; Google UDP DNS is only an optional diagnostic.
- [ ] Verify DNS responses are valid and do not substitute guesses/hardcoded addresses for authentication.
- [ ] Record error codes, elapsed time, source, network stack and remote port separately in a reproducible log.
- [ ] Check DHCP/gateway and consider practical home-network restrictions, firewall and NAT.
- [ ] Investigate whether a **different Xbox 360 network stack** is needed if XNet Retail blocks arbitrary traffic.
- [ ] Maintain transport interfaces allowing future XNet or libxenon/lwIP implementations without rewriting app code.
- [ ] Mark **Milestone M1** passed only when TCP and UDP are reproducibly functional on the actual RGH.

## Phase 2 — Alternative bare-metal network track (conditional)

- [x] Added a boot-only libxenon experiment (source only; no ELF compiled), later archived privately under `PASTACODIGOBASE/XBOX360_EXPERIMENTOS/experimental_xell/`, outside the native XEX build and GitHub source mirror.
- [ ] Install a compatible libxenon/devkitXenon cross-compiler in the developer environment.
- [ ] Compile the boot example separately into ELF32 and load it through already-installed XeLL.
- [ ] Confirm screen, button and exit path on the actual Xbox 360.
- [ ] Inspect the actual libxenon network API/lwIP configuration available in that toolchain version.
- [ ] Implement a minimal DHCP + DNS + TCP + UDP test **on the libxenon side**.
- [ ] Compare network results with the main XDK/XNet `.xex` under identical network conditions.
- [ ] Choose the main network environment based on real test results; libxenon **does not automatically turn an ELF into an XEX**.
- [ ] Avoid NAND flashing or other high-risk console changes merely to test a homebrew ELF.

## Phase 3 — Secure HTTPS on PowerPC

- [x] Consolidated the previously separate `XBOXTERMINAL` MIT XboxTLS13 source into `third_party/xboxtls/`, keeping its license and Xbox 360 native sockets/crypto/X.509 sources.
- [x] Added `src/xboxtls_backend.cpp`: an opt-in TLS 1.3 + bounded HTTP/1.1 bridge using an embedded CA root, no plain HTTP or disabled verification. **Source written; not compiled or independently security-audited.**
- [x] Added `xex.xml`, `src/tls_trust.cpp` and a local Microsoft public-client configuration template. **Hardware compatibility not established.**
- [ ] Independently verify XboxTLS13 certificate-chain and hostname validation, crypto entropy, clock and cancellation behavior on the RGH. Do not treat integration as validated HTTPS.

- [x] Selected and included the MIT XboxTLS13 native TLS 1.3 sources in the main XDK project. **Integration is uncompiled/unverified; keep an independently reviewed alternative (such as mbedTLS) under consideration.**
- [ ] Confirm big-endian PowerPC compiler support and memory footprint.
- [ ] Implement cryptographic randomness from a secure entropy source on the Xbox 360.
- [ ] Provide accurate system time and maintain a safe CA trust store.
- [ ] Implement DNS/socket connections, nonblocking I/O and timeouts under the selected network backend.
- [ ] Support server-name indication, required TLS versions and cipher suites.
- [ ] **Always verify hostname and certificate chain**; no insecure bypass mode for sign-in.
- [ ] Create a real HTTPS request using the transport and HTTP module and parse its response.
- [ ] Complete HTTP error handling, redirects, content length, chunking and size limits.
- [ ] Complete JSON parsing beyond flat OAuth fields and test malformed/untrusted responses.
- [ ] Only mark **M2** complete after a verified HTTPS response from a real remote server on RGH.

## Phase 4 — Authorized Microsoft account sign-in

- [x] Added `src/xbox_account.cpp` worker-thread OAuth Device Code attempt, actual-code-only UI and TLS/WinSock/HTTP diagnostics. **Source written, not compiled or tested online.**
- [x] Added a safe local `auth_config.example.h`; the user's public registered application ID is kept in locally ignored `auth_config.h`.
- [ ] Confirm a real device-code request succeeds on a physical RGH and receives a legitimate Microsoft `user_code`; the existing TCP 10060 timeout may block this.

- [x] Added offline form construction and parsing for standardized OAuth device-code response fields.
- [ ] Confirm that an independently registered client app can obtain **the necessary permissions for xCloud**.
- [ ] Implement secure device-code requests only over validated HTTPS.
- [ ] Display actual `verification_uri` and `user_code` from a server response on TV.
- [ ] Generate a QR code only from a verified URL and/or code, not from a guessed sign-in link.
- [ ] Implement polling with minimum interval, backoff, `authorization_pending`, `slow_down`, cancel/expiry.
- [ ] Support MFA, account restrictions, server errors and correct error messages.
- [ ] Keep refresh/access tokens protected in memory, clear them on exit, and never log or commit them.
- [ ] Implement sign-out and token expiration/refresh if legitimately granted.
- [ ] Prove account authorization for the **specific permitted cloud action**; Microsoft sign-in alone is insufficient.
- [ ] Mark **M3** complete only after authorized sign-in and a valid permitted service request on the RGH.

## Phase 5 — Discovery, entitlement and Fortnite session

- [ ] Determine whether an authorized third-party app can access the relevant Xbox Cloud Gaming endpoints.
- [ ] Discover current game catalog, session region and account entitlements using permitted APIs.
- [ ] Check Fortnite availability in the user's account, market and service region.
- [ ] Create an authorized streaming session and handle queue, wait, heartbeat and cancellation.
- [ ] Parse server-provided signaling and negotiate the actual protocol/version in use.
- [ ] Preserve logs without exposing session credentials.
- [ ] Implement clean session closing, reconnection and logout.
- [ ] Mark **M4** complete only after the service confirms a real Fortnite streaming session.

## Phase 6 — Media negotiation and real-time transport

- [x] Added an RTP header parser for previously decrypted packet buffers — **not WebRTC or SRTP**.
- [x] Added `h264_rtp_receiver.cpp`: Single NAL, STAP-A and FU-A into Annex B access units, with sequence loss detection / IDR resync. **Source only, not compiled; no retransmission/jitter buffer or decoder**.
- [x] Added `xcloud_control.cpp`: bounds-checked handshake/ack, gamepad-change, resolution, video-capabilities and keyframe messages. **Source only; no authorized server connected**.
- [x] Added `xcloud_bridge.cpp`: Xbox native callbacks for a *future secure peer*. **Currently no peer is bound; cannot start a session**.
- [ ] Determine the actual WebRTC/signaling requirements of the authorized session.
- [ ] Evaluate `libpeer` for size, license, compiler and platform portability.
- [ ] Port a DTLS transport using validated cryptographic primitives.
- [ ] Port/configure `libSRTP` for decrypted media transport if required.
- [ ] Port/configure `usrsctp` and negotiated data channels if used for input.
- [ ] Implement SDP, ICE, STUN/TURN, candidate and connectivity selection where required.
- [ ] Implement RTP/RTCP receive loops, timestamps, jitter and loss/retransmission handling.
- [ ] Implement network buffers, retransmission and congestion adaptation.
- [ ] Mark **M5** passed only if the real RGH receives validated media packets from the authorized session.

## Phase 7 — H.264/video decoding and GPU

- [x] Added an H.264 Annex B NAL scanner — **not an H.264 decoder**.
- [ ] Confirm the codecs, profile, level, resolution and packetization the service negotiates.
- [ ] Build a permitted local sample decode proof first, without any cloud service dependency.
- [ ] Research homebrew access to Xbox 360 hardware decoding or feasible software decoding/VMX.
- [ ] Port or write the codec decoder appropriate to the actual negotiated format.
- [ ] Handle RTP depacketization and assemble complete frames correctly.
- [ ] Output frames through Xbox 360 Xenos surfaces/textures with correct byte order.
- [ ] Measure CPU/GPU, 512 MB shared RAM limits, buffer copies and thermal stability.
- [ ] Show a real frame, then continuous video at measured quality/frame rate.
- [ ] Mark **M6-video** passed only after rendering a genuine decoded video stream.

## Phase 8 — Audio and controller interaction

- [x] Added a local XInput snapshot helper capturing controller buttons, triggers and sticks.
- [x] Added `xcloud_input.cpp`: big-endian-safe little-endian gamepad/metadata packet encoding (38/15 bytes) and Xbox controller button mapping. **Source only, still needs physical RGH verification**.
- [ ] Confirm negotiated audio format; evaluate **Opus fixed-point** based on GR33N architecture.
- [ ] Port audio decoder and output via Xbox 360 audio APIs.
- [ ] Synchronize audio/video clocks and jitter buffers.
- [ ] Encode Xbox 360 controller messages in the format explicitly negotiated for the session.
- [ ] Send input via authorized data-channel transport and handle reconnect.
- [ ] Implement rumble/feedback if available, input mappings and deadzones.
- [ ] Measure end-to-end control and audio/video latency.
- [ ] Mark **M6-audio-input** passed only when the actual remote Fortnite session responds to the controller.

## Phase 9 — User experience (inspired by xbox_launcher and PS4/PS3 clients)

- [x] Replaced the diagnostic-first home with a GR33N-inspired four-tab library, game details, favorites, settings and offline tests. **Still needs owner compilation and RGH confirmation.**
- [x] Inspected user-supplied 9 MB PS3 **DeepPlay** development PKG (55 files; SFO title DeepPlay) without executing it; recorded its private UI resource structure. **Not the GR33N app**.
- [x] Added a second procedural Direct3D theme **DeepPlay Midnight**, default on first boot, switchable to **GR33N Neon** in Settings. **Code written, not validated on RGH.**
- [x] Navigation now uses D-pad to select, A to open, X to enter local demo, Y for System; the TCP/DNS probe requires System + X and does not execute at startup.
- [x] Corrected smooth-font coordinates/scaling to match 720p-to-480p rectangle transformations in source.
- [ ] Confirm font readability and actual 720p/480p screen scaling on real hardware.
- [ ] Add user-approved sign-in/device code and QR screens after real OAuth integration.
- [ ] Add a catalog or Fortnite launch button only when it can create a real session.
- [ ] Add states for connecting, queue, session, streaming, reconnect and error.
- [ ] Add user-friendly network, protocol and token status without displaying secret values.
- [ ] Ensure offline/failed network never freezes the interface.
- [ ] Improve accessibility, controller-only navigation, layout and translations.

## Phase 10 — Testing, legal compatibility and release

- [ ] Run local protocol/unit tests without requiring an Xbox Cloud account.
- [ ] Compile in Release / Xbox 360 using the developer's own XDK, then install on RGH.
- [ ] Fix compile/link/runtime errors based on actual RGH test results.
- [ ] Preserve and verify Retail XEX conversion and optional smooth font embedding.
- [ ] Verify no SDK resources or generated Microsoft XPR source are included in public releases.
- [ ] Review licenses before using code from GR33N/XCloud4 (**GPLv3**), XCTV Player (**GPLv2**) or XStreamingDesktop (**MIT**).
- [ ] Pick an explicit license for original XCloud360 code and maintain third-party attribution.
- [ ] Measure long-session stability and memory, error recovery, CPU/GPU temperatures and performance.
- [ ] Verify actual service authorization and compliance with platform agreements.
- [ ] Maintain accurate documentation and avoid presenting diagnostics as completed xCloud streaming.
- [ ] Mark **M7** complete only when authorized Fortnite gameplay renders video and audio **directly on Xbox 360** and responds to the real controller without an intermediary PC.

## Planned compilation and feedback cycle

**Updated 2026-10-20:** The development PC has successfully built and XexTool-verified the one-button PowerPC XEX at the user's request. The owner remains responsible for console-side tests. Do not publish a ZIP or upload private configuration.

1. Developer opens `XCLOUDFORXBOX360.sln` in **Release / Xbox 360** or runs `build_release.ps1`.
2. Developer copies **only the XEX produced in `Release/`** to the RGH, with font data intended to be embedded.
3. Developer reports the first C++/link error or the real TV screen output.
4. When explicitly requested, the native build runs and updates `Release/XCloud360.xex`; no ZIP is produced. Always record compile logs and distinguish Xenia tests from real-console tests.
5. Each passed milestone gets a dated hardware test report.

**Start here:** [STATUS_DO_PROJETO.md](STATUS_DO_PROJETO.md) · [Research references](docs/REFERENCES.md) · [Network failure report](docs/TESTE_RGH3_2026-10-20.md).
