## Latest UI checkpoint: minimal green XEX

Edited the requested XCloud360-GitHub-Source folder: fewer labels, one Fortnite title, green accents and context-dependent controller hints. README clarifies console-direct streaming as an architecture requirement, not a working playback claim. Native PowerPC Release build and Retail verification passed; this UI revision was not visually verified in Xenia or on hardware. SHA256: 4564BADDC065991536FC183EC32DE54E464ACFCB032A5102D8BCB2DA0431C1A2.

---

## Latest checkpoint: native cloud service login (2026-10-21)

Xbox User/XSTS now chains to the xCloud service login endpoint. Xbox scope is requested using the existing private client configuration. Host regression tests and PowerPC Retail build passed. No real cloud login, video/audio or gameplay was verified. See docs/CLOUD_LOGIN_NATIVE_2026-10-21.md. Current XEX SHA256: 7BE20CF855695E5BC18B8DE7B0371D189296B45AF77DFEE67300E685650503FF.

---

## Current checkpoint: Xbox response parser fix (2026-10-21)

**Playback is NOT implemented. This build must not be presented as a playable xCloud client.**
Output: Release/XCloud360.xex
SHA256: 8D73F759064664D56A8981618DAAE19BFEAED592E71ADA6AF3D69EC5C22E1766

- Fixed a confirmed source bug: Xbox User/XSTS used the flat OAuth JSON parser, which rejects the nested DisplayClaims returned by Xbox services. The old synthetic flat replies concealed this defect.
- Switched that exchange to exact-length cJSON object parsing, with rejection of duplicate top-level keys, malformed/trailing JSON and invalid Token fields. Parsed string copies are wiped before release.
- Regression tests now use nested Xbox User/XSTS replies and nested Xbox errors. Host MSVC 2010 test executable passed TLS rejection, HTTP/XErr rejection, nested replies and malformed JSON cases.
- Native Xbox 360 PowerPC Release build and Retail XEX verification passed.
- The green minimal UI now displays a failed Xbox exchange stage and HTTP/XErr values instead of hiding it behind the Microsoft account success message.
- This revision has NOT been run in Xenia or on physical Xbox 360. No actual Xbox User/XSTS authorization or live video/audio was verified.
- No account configuration, client ID, scope setting or certificate verification was changed.

Remaining implementation blockers: no xCloud service login/session manager, no ICE/DTLS-SRTP/SCTP WebRTC runtime, no H.264 video decoding, no Opus playback. Existing RTP/H.264 assembly and controller formatters do not supply these components. The repository currently includes cJSON and XboxTLS, but none of the native WebRTC/media dependencies used by the PS3 reference. PS3 binaries and decoder APIs cannot be directly linked into this Xbox XEX.

The user's latest Xenia failure is not yet diagnosed: the available xenia.log is empty. The parser bug above is confirmed by source and regression tests; it must not be asserted as the cause of the user's observed failure without a corresponding error trace.

---

# XCloud360 — verified development status

**Last verified build:** 2026-10-21, green HD / Segoe UI 42pt / 1080p-first XEX with Xbox User/XSTS HTTPS code and offline tests. **PowerPC compiled and Xenia smoke-tested.** Xbox Live/XSTS tokens and Fortnite streaming are **not** confirmed.  
**Target:** Xbox 360 Xenon PowerPC on physical RGH3, with **no PC relay at runtime**.  
**Project maturity:** experimental native XEX; **Fortnite streaming does not work**.

## Green HD + service-auth build (2026-10-21)

- **Visuals:** green dashboard and sign-in, 1080p preferred (720p/480p fallback), new Segoe 42pt high-density font generated and embedded locally.
- **Build:** Xbox 360 Release XDK build passed, Retail XEX verified by XexTool. Latest SHA-256: `A560698A20E4495F5434DF1ADD93FFEE8DBFE256E6ABE459EE826267709D7616`.
- **Xenia:** homepage and separate login screen passed. A real Microsoft Device Code with a valid server-provided verification URL was displayed after native offline tests.
- **Xbox Live:** source for strict HTTPS Xbox User Token/XSTS exists, but no real Xbox Live scope grant or Xbox token exchange was completed. The default `auth_config.h` was preserved; public example keeps Xbox sign-in opt-in disabled unless the owner's registered client is permitted to request it.
- **Streaming:** xCloud session, WebRTC transport and audio/video decode are not implemented. Fortnite cannot run in this XEX.
- **RGH3:** not retested, previous physical-console TCP 10060 remains unresolved.

See [full green HD test record](docs/GREEN_HD_XBOX_2026-10-21.md).

## Historical source-only checkpoint (subsequently compiled)

At the historical checkpoint, the newly added Xbox HTTPS exchange and offline adapter tests had not yet been built. The **latest compiled revision**, described above, contains them and passed the Xenia device-code smoke test.

**What changed:** Xbox User and service-specific XSTS HTTPS source; exact Microsoft granted-scope gate; restricted Xbox hosts/paths; required JSON contract header; in-memory token wiping; safe HTTP/XErr diagnostics; synthetic bridge and token-exchange tests.

**What is NOT validated:** Xbox-scope consent, valid Xbox User/XSTS responses from live servers, RGH3 physical networking, cloud-session authorization, WebRTC or H.264/Opus decoding. Fortnite cannot be played in this XEX. Do **not** report a source-only implementation as a working Xbox authentication or game-streaming client.

See [latest detailed technical report](docs/HARISSA_UI_XBOX_HTTPS_2026-10-21.md).

## Previous verified source milestone (2026-10-21)

The screenshot's `XSTS/WEBRTC/MEDIA NOT IMPLEMENTED` text describes unimplemented cloud-streaming components, **not** an Xenia crash or a network error. Microsoft OAuth token approval is not sufficient to obtain Xbox Live/XSTS permissions.

- The application currently requests basic Microsoft scopes (`openid profile offline_access`) and its token is securely erased when the worker exits.
- The native UI now checks whether the genuine OAuth token response **explicitly** grants `XboxLive.signin`. The initial blocker is `XBOX LIVE SCOPE NOT REQUESTED` until an owned application requests and receives that scope.
- Native source for bounded Xbox User Token/XSTS request preparation has been added with offline tests; real service exchanges are **not implemented**.
- The latest XEX builds and runs in Xenia; it requested a real device-code response in the new test. No new authorized XSTS exchange or Fortnite stream has occurred.
- On stock, unmodified Xbox 360, unsigned XEX homebrew won't normally run; the intended test hardware is RGH/RGH3 with the network limitations still unproven.

Latest release SHA-256: `D787192AD103EDE6C462E45E99D8A6FF0D0C2DCAF56271D40E4F67A5C1534AB7`. See [scope preflight and hardware report](docs/XBOX_AUTH_SCOPE_PREFLIGHT_2026-10-21.md).

## Latest user-facing release

The single game currently available for selection is **Fortnite**, linked as a reference to:

`https://play.xbox.com/products/BT5P2X999VH2/fortnite`

Pressing **A** runs one sequence in the native terminal:

| Step | Maximum measured setup progress | Actual action |
| --- | ---: | --- |
| 1 | 10% | Select Fortnite and display its URL |
| 2 | 20% | Run offline parser, JSON, and TLS cryptographic tests |
| 3 | 35% | Diagnose DNS, TCP and verified HTTP; DNS failure is not disguised as success |
| 4 | 45% | Attempt Microsoft TLS 1.3 HTTPS Device Code request |
| 5 | 60% | Present a genuine Microsoft verification URL and code, with server-issued countdown |
| 6 | 70% | Record an actual Microsoft OAuth authorization result, if one arrives |
| 7 | **Blocked** | Xbox Live/XSTS, authorized xCloud session, WebRTC, video/audio decode, controller return and real gameplay |

**The percentages represent client setup milestones only**, not the percentage of a playable game. A nonexistent streaming session cannot reach 100%.

After Microsoft issues a code, the app allows at least **30 seconds before its first token-poll request**. Subsequent polls obey server intervals, including `slow_down`, and never extend the server's expiry. The code remains on screen while the server genuinely waits for account authorization. OAuth HTTP 400 can be an expected `authorization_pending` response, but a different 400 error must still be investigated; no success is fabricated.

## Test record on Xenia Canary

| Check | Evidence |
| --- | --- |
| Native PowerPC compilation | **PASS** — latest build from Xbox 360 XDK, exit 0 |
| Retail compressed `.xex` conversion | **PASS** — XexTool verification |
| Fortnite homepage and A-triggered Microsoft login in Xenia | **PASS** — captured `xenia_homepage_fortnite_20261020.png` and `xenia_fortnite_one_button_flow.png` |
| Offline parser, JSON and TLS crypto tests | **PASS** — native XEX self-test |
| DNS over TCP with Cloudflare fallback | **PASS** in preceding Xenia network test |
| Microsoft HTTPS / real Device Code issuance | **PASS** in previous native Xenia build after correcting TLS 1.3 non-PSK early secret |
| Token authorization | **PASS in latest Xenia test at the OAuth state-machine level** — a genuine HTTP token response was accepted; current worker erases the token on exit and Xbox Live/XSTS was not tested |
| Press-A one-button flow in latest build | **PASS with Xenia keyboard-as-gamepad**: transitioned from Fortnite homepage to code panel, displayed a server-issued code, and later reached 70% login status |
| Real Xbox 360 RGH3 test of latest XEX | **NOT PERFORMED** |
| Xbox Cloud session / Fortnite video, audio or controller | **NOT IMPLEMENTED** |

**Current XEX file:** `Release/XCloud360.xex`.  
**Current SHA-256:** `D787192AD103EDE6C462E45E99D8A6FF0D0C2DCAF56271D40E4F67A5C1534AB7`.

The requested legacy `login.live.com/oauth20_remoteconnect.srf` page belongs to a different Live OAuth flow; the current v2 code must be entered at the server-returned `verification_uri` (`https://www.microsoft.com/link` in the Xenia test). See [homepage/login technical record](docs/HOMEPAGE_MICROSOFT_LOGIN_2026-10-20.md).

**Final build note:** the post-login UI was revised after the captured 70% test so that it displays the remaining xCloud milestones, rather than reiterating the legacy sign-in link. Final compressed Retail XEX compiled successfully; this final visual-only change was not re-authorized against Microsoft. 

The Xenia test harness and images reside privately in `PASTACODIGOBASE/ARQUIVOS_ANTIGOS/`; they are excluded from GitHub, as are binaries and account configuration. Build and test results are not equivalent to a physical RGH3 compatibility test.

## Important historic hardware observation

The user previously measured error **10060**, source **8**, while connecting from physical RGH to `www.xbox.com:443` and `1.1.1.1:443`. This is a timeout at the application's TCP-select stage, **not proven to be a DNS error**. The current external DNS/TLS fixes have not been retested on physical hardware.

## Confirmed fixes and limitations

- Verified PowerPC Retail build and embedded font.
- Native DNS fallback: Xbox XNet resolver first; numeric public UDP/TCP DNS queries if necessary.
- TCP diagnostics distinguish writable sockets from protocol-verified HTTP. A writable socket does not prove HTTPS or streaming.
- TLS 1.3 client made a real authenticated HTTPS Microsoft Device Code request on Xenia after fixing non-PSK early-secret derivation; added SHA-384/RSA certificate support and tests.
- Microsoft OAuth worker receives genuine codes, polls in the background, respects response validity and keeps tokens out of logs and source code.
- **No supported native WebView/browser or HTML5 engine**, no Xbox Live/XSTS service authorization, no xCloud signaling/WebRTC, no H.264 frame decoding or audio path, no remote controller gameplay. Clicking or displaying a URL does not start Fortnite.

## What to test next

1. Launch the latest `Release/XCloud360.xex` in Xenia, ensure your Xbox controller is recognized, then press **A**.
2. Report the numbered log: steps 1–5, setup percentage and whether a **real verification URL + code** stays visible.
3. Use your own phone/PC to visit the URL returned by **Microsoft**, not a synthetic code. If a token poll returns a failure, share only its **error name and HTTP status**, never your code, password or token.
4. After Xenia results, repeat on physical RGH3. If error `10060` returns, identify whether the console network stack is blocking external traffic.
5. Develop and validate authorized xCloud sessions, negotiated WebRTC transport and codecs as separate future milestones.

**No game-running claims, no ZIP, no GitHub push and no SDK/PS3 resources in the public source mirror.** See [ROADMAP.md](ROADMAP.md), [README.md](README.md) and [the one-button report](docs/FORTNITE_ONE_BUTTON_2026-10-20.md).
