# XCloud360 — Fortnite homepage and Microsoft device-code login

**Date:** 2026-10-20  
**Target:** Xbox 360 Xenon/PowerPC native XEX on RGH/RGH3.  
**Test environment:** Xenia Canary on Windows. Physical RGH networking remains untested for this build.

## New UI navigation

1. **Home** — a separate full-screen game selector showing one entry, Fortnite, with the URL `https://play.xbox.com/products/BT5P2X999VH2/fortnite`.
2. **Fortnite / Microsoft sign-in** — pressing **A** goes to a separate sign-in screen and automatically runs the local protocol tests, DNS/TCP diagnostics and the verified-HTTPS Microsoft Device Code request.
3. **Device code** — only a real `user_code` from Microsoft's server appears, prominently alongside its **corresponding** `verification_uri`, expiry timer and polling status. Authorization happens via a phone/PC browser; no Xbox browser is provided.
4. **Post-authorization** — if Microsoft actually grants an OAuth access token, the screen reports `MICROSOFT DEVICE LOGIN SUCCESSFUL` and marks the next xCloud/XSTS/media stages as not implemented. The test token is cleared when the worker ends.
5. **Navigation** — **B** cancels sign-in and returns to the homepage; **A** retries after failure without restarting an active code; **X** reruns network diagnostics, **Y** reruns offline tests, BACK exits.

A native Xbox 360 XEX does **not** launch or embed `play.xbox.com` in a modern HTML5 browser. A real Xbox Cloud Gaming session is not available: there is no authorized XSTS/xCloud session, WebRTC transport, video/audio decoder or gameplay controller channel. The progress display therefore stops at 70%, not a fabricated 100%.

## The requested login.live.com URL

Requested Xbox legacy login page: `https://login.live.com/oauth20_remoteconnect.srf`.

The existing application is registered for an **Entra/Microsoft identity v2** public application ID (GUID) and requests device codes from:

- `POST https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode`
- `POST https://login.microsoftonline.com/consumers/oauth2/v2.0/token`

Microsoft's v2 device-code response contains a `verification_uri` **specifically tied to that code**. In the Xenia test, it returned `https://www.microsoft.com/link`. The **legacy** `login.live.com/oauth20_remoteconnect.srf` page uses a different Live OAuth flow. It cannot safely be substituted for the v2-issued `verification_uri` without implementing that separate provider using the developer's own legitimately registered legacy client identity. The requested link is documented and displayed as an explicitly labeled legacy reference **before a code is issued**, but the real code screen uses the URI returned with the real code.

Reference: Microsoft identity platform OAuth Device Code documentation and legacy XAL `oauth20_remoteconnect.srf` endpoint descriptions. Do not borrow third-party client IDs, cross-use codes, bypass TLS or ask users to enter passwords in the XEX.

## OAuth polling reliability fixes

- A 30-second grace period after the device-code HTTP response precedes the first token check. The server's `expires_in` value still controls the code's actual lifetime.
- After any blocking HTTPS token check, the next poll timer is **rebased from response completion**, preventing accidental rapid polling when TLS roundtrips take several seconds.
- `authorization_pending` (HTTP 400) keeps the code and URL on screen, while `slow_down` increases the interval. Invalid/expired/declined codes stop the flow with meaningful error names.
- The display reports `POLL #N` and `NEXT CHECK: N SEC`. OAuth tokens and credentials are not included in debug logs or the GitHub source mirror.
- The native offline test uses a simulated HTTPS server and covers both non-verified transport rejection and valid code, authorization_pending, slow_down, polling after delayed HTTP completion, and a synthetic success.

## Actual Xenia test evidence

| Test | Observed result |
| --- | --- |
| Xbox 360 PowerPC compilation and compressed Retail XEX | **PASS** using XDK and XexTool |
| XEX boot and Fortnite homepage | **PASS**, local screenshot `xenia_homepage_fortnite_20261020.png` |
| Xenia keyboard-as-controller A from homepage to login | **PASS**, launched with `--hid=winkey --keyboard_mode=1` and the configured A key |
| Microsoft TLS 1.3 Device Code request | **PASS**, genuine `https://www.microsoft.com/link` verification URL and server-issued code displayed |
| Server-issued expiry | **PASS**, 870 seconds remaining at first screenshot; full timeout not exhaustively tested |
| OAuth token authorization | **PASS at the application state-machine level** — later Xenia capture showed `MICROSOFT DEVICE LOGIN SUCCESSFUL`, 70% and a successfully processed token response |
| Xbox Live XSTS access token / xCloud session / Fortnite stream | **NOT IMPLEMENTED** |
| Actual Xbox 360 RGH3 gameplay and networking | **NOT TESTED** |

The successful OAuth response was accepted by the XEX's TLS-protected token parser, and the token was erased on worker exit; it was **not independently redeemed against Xbox Live/XSTS**. Consequently, the successful sign-in is not evidence that Xbox Cloud Gaming streaming is functional or authorized.

Private screenshots and test scripts are kept only in `REFERENCE_BASE/ARQUIVOS_ANTIGOS/`, not published.

## Next functional milestones

- Test all current DNS/TCP/TLS components on physical RGH3, where an older build had error 10060.
- If necessary, support the legacy Live device-flow **as a separate configured provider**, with its own authorized client ID, response format, token endpoint and associated verification URI.
- Implement permitted Xbox Live user-token/XSTS acquisition, with secure token handling and appropriate service scopes.
- Obtain a documented/authorized xCloud session, add complete ICE/STUN/TURN/DTLS/SRTP WebRTC transport, actual H.264/Opus decoding and Xbox 360 controller return channel.
- Measure performance and memory usage on real Xenon hardware.

**No ZIP, no GitHub push, no media-streaming claim.**
