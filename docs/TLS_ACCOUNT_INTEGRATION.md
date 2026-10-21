# XCloud360 — TLS 1.3 and Microsoft account integration (source-only)

**Target:** one Xbox 360 Xenon PowerPC XEX using the Xbox 360 XDK, launched directly on RGH/RGH3 without a PC proxy.

**Date:** 2026-10-20. **Status: not compiled or tested on real Xbox 360 after integration.** This is **not** a functioning Xbox Cloud Gaming or Fortnite client.

## Consolidation

The former standalone `XBOXTERMINAL/` proof-of-concept has been **incorporated into the main XCloud360 source tree**. We retained the reusable, MIT-licensed XboxTLS13 library and adapted its socket diagnostics, HTTPS client and Microsoft device-code logic into the main app. The duplicate standalone `main.cpp`, temporary Visual Studio cache, build artifacts, XEX, separate solution and duplicate D3D display were not carried into the main build.

| Root location | Function |
| --- | --- |
| `third_party/xboxtls/include/xboxtls/` | XboxTLS13 public C++ headers |
| `third_party/xboxtls/src/` | 20 Xbox 360-compatible TLS 1.3, crypto, X.509, verification and socket C++ implementation files |
| `third_party/xboxtls/external/xkelib/` | Required compatibility header files; no compiled binaries/SDK redistributions |
| `third_party/xboxtls/LICENSE` | Upstream XboxTLS13 MIT license; preserve notices |
| `src/tls_trust.cpp`, `src/tls_trust.h` | Embedded DigiCert Global Root G2 trust-store data from the experiment |
| `src/xboxtls_backend.cpp`, `src/xboxtls_backend.h` | Verified-TLS **candidate** and HTTP/1.1 form POST bridge for two Microsoft OAuth endpoints, guarded by `secure_http::Transport` |
| `src/xbox_account.cpp`, `src/xbox_account.h` | Worker-thread device-code login state, synchronization and controller UI snapshots |
| `src/xbox_device_flow.cpp` | OAuth device-code request, server interval/backoff, cancel/expiry, volatile RAM token storage and erasure |
| `src/app.cpp` | **Settings → Microsoft account → A** initiates sign-in on the console if configured. Actual verification URI and user code appear **only on a genuine server response** |
| `auth_config.example.h` | Public config template; copy to local `auth_config.h` to build. **No password or client secret required** |
| `xex.xml` | Xbox 360 XEX media/privilege manifest carried over from the standalone test |

The standalone source was archived by reference in this document. The physical `XBOXTERMINAL` folder has been deleted **only after** hashing/copying reusable source and independently verifying the VS project references and GitHub mirror.

## What the new code attempts

1. Check the user's public application ID locally.
2. On **A** in account details, create a background worker, initialize Xbox XNet and TLS 1.3, validate a Microsoft HTTPS certificate against the embedded root, and send a device-code request.
3. If a server-provided `verification_uri` and `user_code` arrive, display them on the Xbox 360 screen. The user authorizes in the **official Microsoft website** using a phone or other device.
4. Respect server polling intervals, backoff (`slow_down`), expiry and cancellation.
5. Display account-token receipt only if the server returns a valid response. The worker wipes its tokens on exit; **there is no Xbox Live/XSTS or xCloud session integration yet**.
6. Show TLS, WinSock and HTTP error codes on failure. The app remains in its native UI; it does not automatically exit on an HTTPS error.

All sign-in requests are **opt-in**, not performed at startup. No password is typed into or stored by the XEX. There is no plain-HTTP fallback and no certificate-verification bypass.

## Last known tested issues and new unverified work

- On real Xbox 360 RGH, `www.xbox.com:443` and direct `1.1.1.1:443` timed out with WinSock error **10060**, diagnostic source **8**. DNS resolved one IPv4 address. The *root cause is not known*. Changing DNS alone did not prove a fix.
- The separate `XBOXTERMINAL.xex` returned code **3** in Xenia: its first HTTPS request failed. The original log did not identify whether the failure was during socket startup, DNS, TCP, TLS or HTTP. **Xenia is not a substitute for RGH testing**.
- The MIT XboxTLS13 engine has now been referenced in the main Xbox 360 project, along with a bounded HTTP transport and a **background-thread** login path. **None of those new paths has been proven to compile or connect on a physical console.**
- The embedded trust store contains a single named root. It may not match a current Microsoft service certificate chain and must be maintained. A validation failure is correct; do **not** disable certificate verification to proceed.
- The XboxTLS13 library's chain, hostname, randomness, date/time and cipher-suite implementation must still undergo security review before using a real account. The source adapter assumes `XboxTlsClient::connect()` enforces certificate and host validation; this assumption requires explicit verification on hardware and protocol traces.
- Socket connect/DNS have finite timeouts in the migrated code. The TLS handshake itself must also be tested for predictable cancellation and timeouts.
- Microsoft Device Code OAuth alone does **not** authorize arbitrary Xbox Cloud Gaming requests. Service entitlements, valid public API permissions, Xbox Live/XSTS, session discovery, WebRTC, audio/video decoding and game input remain separate milestones.

## Build from a clean GitHub checkout

A valid Xbox 360 XDK installation, SDK-provided font tools/assets and permitted Retail packaging environment are required.

```powershell
Copy-Item .\auth_config.example.h .\auth_config.h
# Edit auth_config.h with your own registered Microsoft public client ID.
.\build_release.ps1
```

The actual local `auth_config.h` is deliberately ignored by Git. The public `auth_config.example.h` is safe to publish and intentionally contains an invalid placeholder. No Microsoft password or token should ever be committed.

**Owner builds**; the assistant did not compile, launch, generate ZIP or change `Release/` while performing this consolidation.

## Planned verification order

- [ ] Confirm the unified XEX compiles with the Xbox 360 XDK and shows the native game library.
- [ ] Confirm the library and local protocol self-tests continue working.
- [ ] Confirm Settings → Microsoft account shows the start button, and cancellation works.
- [ ] Record precise XNet, DNS, TCP, TLS and HTTP failures on physical RGH, especially whether `10060` persists.
- [ ] Confirm proper certificate chain/hostname validation against real responses.
- [ ] Receive a genuine device authorization code with a registered client ID.
- [ ] Confirm authenticated MSA token receipt **without persistence** and safe exit.
- [ ] Research permitted Xbox Live/XSTS and xCloud service authorization.
- [ ] Implement the actual xCloud session/WebRTC and optimized codecs.

## Acknowledgements, licenses and limitations

TLS 1.3 source adapted from [Ascension Digital Technologies / Xbox360-TLS-v1.3](https://github.com/Ascension-Digital-Technologies/Xbox360-TLS-v1.3), distributed under the upstream **MIT** license in `third_party/xboxtls/LICENSE`. Compatibility headers and dependencies must be reviewed individually before publication; the proprietary Xbox 360 XDK itself is **not** included. cJSON remains covered by its own MIT notices. GR33N PS3 source (GPL-3.0), the DeepPlay PS3 PKG, game art, Xbox firmware and title binaries are **not** copied into this integrated source tree.

The project is independent of Microsoft, Xbox and Epic Games.
