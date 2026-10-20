# XCloud360 — Complete development roadmap

**Platform:** real Xbox 360 RGH/RGH 3, native Xenon PowerPC, no intermediary PC.  
**Goal:** create an authorized Xbox Cloud Gaming session, stream Fortnite video/audio to the Xbox 360, and send the Xbox 360 controller input back to that session.  
**Last confirmed real-console network result:** Xbox DNS resolved 1 IPv4 address, but TCP connections to `www.xbox.com:443` and `1.1.1.1:443` both timed out with error `10060`, source `8` (`TCP_SELECT`).  
**Important:** this project is **NOT a functional xCloud client**. Source implementations are not equivalent to successful real-console tests.

Checklists have three distinct meanings:
- **Implemented source:** a source file exists, but it may not have been compiled or tested.
- **Validated on RGH:** the required feature demonstrably ran on physical hardware.
- **Blocked / pending:** missing functionality, unknown service access, or failed testing.

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

- [x] Added isolated `experimental_xell/` boot-only libxenon example (source only; no ELF compiled).
- [ ] Install a compatible libxenon/devkitXenon cross-compiler in the developer environment.
- [ ] Compile the boot example separately into ELF32 and load it through already-installed XeLL.
- [ ] Confirm screen, button and exit path on the actual Xbox 360.
- [ ] Inspect the actual libxenon network API/lwIP configuration available in that toolchain version.
- [ ] Implement a minimal DHCP + DNS + TCP + UDP test **on the libxenon side**.
- [ ] Compare network results with the main XDK/XNet `.xex` under identical network conditions.
- [ ] Choose the main network environment based on real test results; libxenon **does not automatically turn an ELF into an XEX**.
- [ ] Avoid NAND flashing or other high-risk console changes merely to test a homebrew ELF.

## Phase 3 — Secure HTTPS on PowerPC

- [ ] Choose and port a maintained TLS stack (mbedTLS is a candidate from GR33N, not currently integrated).
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

- [x] Reworked the XEX home screen with Xbox-green cards and a Fortnite status tile.
- [x] Press A for roadmap, Y for Fortnite status, X for optional network diagnostics.
- [ ] Confirm font readability and 720p/480p screen scaling on real hardware.
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

The project owner compiles. The assistant edits source files only.

1. Developer opens `XCLOUDFORXBOX360.sln` in **Release / Xbox 360** or runs `build_release.ps1`.
2. Developer copies **only the XEX produced in `Release/`** to the RGH, with font data intended to be embedded.
3. Developer reports the first C++/link error or the real TV screen output.
4. Fixes are applied to source files; **no build, ZIP or overwrite of `Release/` is performed by the assistant**.
5. Each passed milestone gets a dated hardware test report.

**Start here:** [STATUS_DO_PROJETO.md](STATUS_DO_PROJETO.md) · [Research references](docs/REFERENCES.md) · [Network failure report](docs/TESTE_RGH3_2026-10-20.md).
