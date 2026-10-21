# Xbox Live / XSTS preflight and the Fortnite streaming blocker

**Date:** 2026-10-21  
**Application:** XCloud360, native Xbox 360 Xenon/PowerPC XEX for modified RGH/RGH3 hardware.  
**Status:** preflight and offline request builders added; **the code still cannot stream Fortnite**.

## What the screenshot really shows

The UI correctly received and parsed an authenticated Microsoft OAuth token after the device-code approval. This **does not** prove permission to use Xbox Live services, retrieve Xbox User and XSTS tokens, create a cloud session, or stream video. The previous text `NOT IMPLEMENTED` is a truthful capability barrier, **not an exception, failed XEX launch or Xenia error**.

In previous versions, `XboxAccount::perform()` called `Flow::configure()` with **`openid profile offline_access`**. Those are basic Microsoft identity scopes. Xbox services authentication requires a Microsoft token with authorized Xbox scopes such as `XboxLive.signin` and an additional token exchange. Xbox Cloud Gaming session permission is separate.

## Changes in this source revision

- Added `src/xbox_live_auth.h` and `src/xbox_live_auth.cpp`, compiled as native PowerPC. These contain a capability/precondition sequence and **offline-only** JSON request construction for official Xbox User Token and general XSTS endpoints. No real network call is performed and no Xbox user token is fabricated.
- Added tests for exact scope recognition, readiness ordering, bounded token characters, correct Xbox JSON formatting, and rejecting quotes/escapes from untrusted token input.
- The login worker still preserves the **working default** Microsoft Device Code flow. There is an explicit **opt-in** setting in `auth_config.example.h`:
  `#define XCLOUD360_ENABLE_XBOX_LIVE_SCOPE 0`
- Only after the owner has a properly registered/authorized Microsoft public application may the private `auth_config.h` set it to `1`. This **requests** `XboxLive.signin XboxLive.offline_access`; the server can still reject consent or refuse the permission. Do not enable it as a random fix, and never borrow another application's client ID.
- The successful OAuth token response is examined for its explicitly granted `scope`. The status flag records whether `XboxLive.signin` actually appears in that field; if the field is absent, Xbox permission is **not verified**. No token or user code is exported to the log.
- The post-login screen now reports the **first unfulfilled Xbox requirement**. With the original basic scopes, the next blocker is `XBOX LIVE SCOPE NOT REQUESTED`. Further steps include Xbox User Token, XSTS, session, WebRTC, actual H.264/audio decoders and controller return path.
- Corrected the flow cleanup to clear any in-memory granted-scope metadata at cancellation; the existing OAuth access token is wiped after the worker exits. A future real Xbox User Token exchange requires redesigning this ephemeral hand-off safely.
- Kept the game-selection homepage and Microsoft code approval unchanged.

## The Microsoft GDK's service authentication sequence

Microsoft documents the following service flow, **subject to permissions and registration**:

1. Authenticate a Microsoft user and request **XboxLive.signin** consent (not just OpenID).
2. Use the authorized Microsoft access token to request an Xbox User Token at `https://user.auth.xboxlive.com/user/authenticate`.
3. Exchange that Xbox User Token for a service-appropriate **XSTS** token at `https://xsts.auth.xboxlive.com/xsts/authorize`.
4. Use the approved token for permitted Xbox services. **There is no guarantee that the public xCloud streaming service grants unofficial clients the required session access.**
5. For actual Fortnite streaming, implement full authorized cloud signaling, ICE/STUN/TURN, DTLS/SRTP/WebRTC media, hardware-appropriate H.264/audio decoding and Xbox 360 gamepad data transport.

Reference: https://learn.microsoft.com/en-us/gaming/gdk/docs/services/fundamentals/s2s-auth-calls/service-authentication/live-website-authentication

**Important:** the offline request builders produce JSON only; they are not an implementation of Xbox Live/XSTS authentication and are not integrated with the TLS network transport. The example generic XSTS relying party is **not** an xCloud streaming authorization.

## Xbox 360 hardware support

- **Unmodified Retail Xbox 360:** will not normally run unsigned homebrew XEX applications. Xbox Cloud Gaming officially supports **Xbox One and later consoles**, not Xbox 360.
- **RGH/RGH3/JTAG Xbox 360:** the native Retail-encrypted-format homebrew XEX can be launched by appropriate homebrew loaders. However, previous physical-RGH network tests timed out on TCP with error 10060; the recent Xenia successes do not demonstrate reliable physical-console DNS, TCP, TLS or low-latency decoding.
- **Xenia emulator:** useful for validating PowerPC compilation, rendering, offline tests, and some TLS behavior. Its network compatibility is not equivalent to RGH hardware.

Official platform information: https://news.xbox.com/en-us/2025/12/19/getting-started-with-xbox-cloud-gaming/

## Validation checklist

- [x] Source backed up with SHA-256 verification under private `PASTACODIGOBASE/ARQUIVOS_ANTIGOS/BEFORE_XBOX_SCOPE_PREFLIGHT_20261021/`.
- [x] Xbox Live scope and offline token-request builders integrated with the PowerPC XDK project.
- [x] Xbox request builder and authorization-gate checks run in the XEX's offline self-test.
- [x] PowerPC Retail build completes with XexTool verification.
- [x] New build boots into the Fortnite homepage in Xenia.
- [x] Xenia keyboard-A login test verified: the native XEX showed an actual Microsoft-issued verification URI and code in the normal waiting screen. Offline tests had to pass for the app to reach this screen.
- [ ] New build post-authorization blocked-step display after account consent; not retested, because this test did not ask the user to approve another device code.
- [ ] Authorized Microsoft/Xbox scope consent from a registered application.
- [ ] Actual Xbox User Token/XSTS network exchange and Xbox Live service permission.
- [ ] Physical RGH network validation, authorized cloud session and streaming.

No ZIP, no GitHub push, no stored login tokens, no fake Fortnite session.
