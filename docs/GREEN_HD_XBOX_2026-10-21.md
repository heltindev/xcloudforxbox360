# XCloud360 green HD interface and Xbox authentication build

**Updated:** 2026-10-21  
**Target:** modded Xbox 360 Xenon PowerPC running a native XEX (RGH/JTAG).  
**Runtime architecture:** no intermediary PC; Windows is only used to develop and test.  
**Fortnite streaming:** **not functional or implemented**.

## Summary of improvements

- Replaced the Harissa-inspired orange/graphite theme with an independently authored **emerald-green dashboard**, improved contrast, larger labels, tighter panel borders, a new green abstract city illustration and more legible navigation.
- Native rendering first requests **1920x1080**, then falls back to **1280x720** or **640x480**, preserving logical UI coordinates. Actual physical console display negotiation and FPS have **not been tested**.
- A real **42-point Segoe UI font atlas** was rasterized on the developer's own Windows machine using the installed Xbox 360 SDK FontMaker source and XDK bundler. It is embedded into the XEX via `embed_font.ps1`; display size is adjusted by the atlas density so edges are smoother rather than simply magnified. The original 24-point XDK Segoe UI resource remains a fallback.
- The new `generate_hd_font.ps1` regenerates the high-density atlas locally on systems with a licensed Xbox 360 SDK, Visual Studio 2010/MFC and Segoe UI installed. Its generated font files are **excluded from GitHub**: `.abc`, `.tga`, `.rdf`, `.xpr` and the generated XPR C++ translation unit stay in private/local build folders. Do not publish or distribute font files.
- Retained the Fortnite-only homepage, separate Microsoft Device Code login screen, controller navigation, URL/code expiry information and explicit unavailable streaming status.

## Next Xbox Live/XSTS step

The latest code contains native HTTPS exchange support for `user.auth.xboxlive.com/user/authenticate` and `xsts.auth.xboxlive.com/xsts/authorize`. These requests require **verified TLS, an explicit Xbox Live sign-in grant**, strict host/path allowlists and properly formatted Xbox JSON. The background worker erases token buffers. Offline synthetic tests cover successful formatting/response, unverified HTTPS rejection, and Xbox HTTP 401/`XErr`.

However, **nothing in this test means the Xbox service granted the user's account an XSTS credential**:

1. The user's private `auth_config.h` was **not edited**. By default, `XCLOUD360_ENABLE_XBOX_LIVE_SCOPE` is disabled or absent, preserving the known-working Microsoft identity flow.
2. If an **owned, legitimately permissioned** app registration exists, the owner can opt in by setting `#define XCLOUD360_ENABLE_XBOX_LIVE_SCOPE 1` in the private `auth_config.h`, then rebuilding. This merely **requests** `xboxlive.signin`. Microsoft's server may reject it.
3. The login's OAuth token response must **explicitly grant** the Xbox sign-in scope before the XEX attempts Xbox User Token/XSTS requests. The status screen now distinguishes the disabled scope from a requested-but-not-granted scope, and displays **"Xbox HTTP requests have NOT been sent"** when blocked.
4. Xbox User/XSTS HTTPS host certificates inspected using Windows' trusted chain for both public endpoints currently terminate at **DigiCert Global Root G2**, which is already embedded in the native trust set. This does **not** prove the handshake works on Xenia or physical RGH.
5. Even a valid service XSTS token does **not** grant access to xCloud's proprietary session APIs, and the app cannot start Fortnite without an authorized cloud session, ICE/DTLS/SRTP, codec decoding and the input return channel.

Do not use another app's client ID or disable certificate validation to overcome a Microsoft authorization failure. Device-code secrets, passwords and bearer tokens are never included in source, debug logs or GitHub mirrors.

## Validation performed (Xenia Canary)

| Test | Result |
| --- | --- |
| PowerPC compilation of Xbox live exchange and offline selftests | **PASS**; Xbox 360 XDK Release |
| XexTool Retail XEX conversion | **PASS** |
| Native two-panel green homepage and separate sign-in page | **PASS** on Xenia |
| 1080p display rendering selected by XEX | **PASS** in Xenia; 720p/480p fallback paths not yet exercised |
| Embedded high-density Segoe 42pt font loader | **PASS**, Xenia screenshot shows smooth font rather than enlarged bitmap |
| Offline OAuth, Xbox request exchange, controller bridge and RTP/H264 protocol checks | **PASS** to the extent the native startup selftest reached the online device-code stage; **synthetic tests only** |
| Microsoft verified HTTPS Device Code issuance | **PASS** in Xenia; real server verification URL and user code displayed; code deliberately not included in this report |
| Current-build completed Microsoft token approval | **NOT TESTED** with a consenting owner in this session |
| Live Xbox User Token / XSTS exchange | **NOT TESTED** (no confirmed Xbox sign-in grant) |
| Fortnite / xCloud service session and WebRTC decoding | **NOT IMPLEMENTED** |
| Physical RGH3 networking and video performance | **NOT TESTED** (historical TCP 10060 remains relevant) |

The previous `XBOX LIVE SCOPE NOT REQUESTED` screen was a **true permission precondition**, not evidence of a failed video decoder or damaged source. This revision clarifies it but does **not** bypass it.

## Private test artifacts

Screenshots, XDK compiler logs, prior-version snapshots and generated XDK font artifacts are stored privately under:
`PASTACODIGOBASE/ARQUIVOS_ANTIGOS/`, `assets/Fonts/` and `obj/`.

Binary output: `Release/XCloud360.xex`  
SHA-256: `A560698A20E4495F5434DF1ADD93FFEE8DBFE256E6ABE459EE826267709D7616`

No ZIP is created. The source mirror is updated with code, documentation and the high-density font generator **script only**, not fonts or binaries.

## References

- Microsoft Gaming GDK service auth: https://learn.microsoft.com/en-us/gaming/gdk/docs/services/fundamentals/s2s-auth-calls/service-authentication/live-website-authentication
- Microsoft identity platform device-code flow: https://learn.microsoft.com/en-us/entra/identity-platform/v2-oauth2-device-code
- Local `docs/HARISSA_UI_XBOX_HTTPS_2026-10-21.md` and `ROADMAP.md` (separate implemented source, verified Xenia, and missing real-console stages).
