# Development update: Unified XboxTLS13 and Microsoft sign-in (source milestone)

**October 20, 2026** — Source-focused, experimental Xbox 360 PowerPC release notes.

> **Important: no playable Xbox Cloud Gaming or Fortnite yet.** The current source changes have **not been compiled or successfully tested on a real RGH**. Microsoft OAuth device-code login has been *wired into the source* but is **not verified as functional**.

## Added / updated

- Consolidated the **XBOXTERMINAL** experimental Xbox 360 TLS code into the main XCloud360 XEX source tree, removing the need for a second executable.
- Integrated 20 native C++ TLS 1.3/cryptography/X.509 sources from the MIT-licensed XboxTLS13 reference, keeping the original license notice.
- Added XboxTLS13 to the root Visual Studio Xbox 360 project, with its headers, required compatibility headers and XDK-linked networking libraries.
- Added an embedded trust-store resource to the main XEX project; **must be updated/audited for current server chains**.
- Implemented a bounded HTTPS/HTTP/1.1 bridge for the documented Microsoft OAuth device-code endpoints. It requires certificate and hostname validation; no insecure HTTP fallback.
- Added a **controller-initiated Microsoft Device Code UI** to the native app. A worker thread handles requests and polling without blocking Direct3D; tokens remain private to the worker and are erased.
- Added TLS stage/WinSock/HTTP error display to the account panel.
- Added `auth_config.example.h` for independently registered public client IDs. Actual local app configuration is excluded from Git.
- Preserved the existing dashboard themes, Fortnite status, offline controller demo and optional network tests.
- Moved XEX manifest settings to the root `xex.xml` for future RGH validation.

## Known issues / missing work

- Previous real RGH network results: **10060**, source **8**, on both Xbox.com:443 and 1.1.1.1:443. The root cause is unknown, and may prevent TLS altogether.
- Standalone XBOXTERMINAL Xenia test exited with code **3**, indicating the first HTTPS exchange failed; not evidence of an Xbox Live authorization error.
- Modern TLS 1.3 certificate chain validation, entropy, hostname checking, time handling and XDK linker compatibility **must still be independently tested**.
- A successful Microsoft login is **not demonstrated**. Xbox Live/XSTS tokens and xCloud API authorization are not implemented.
- WebRTC/DTLS/SRTP, audio and H.264 decoding, Fortnite session management and latency tuning are still required.
- Xenia is a secondary debugging aid, not a substitute for tests on real RGH3 hardware.

## Build and test

From the project's root, use your own **registered** Microsoft public app ID:

```powershell
Copy-Item .\auth_config.example.h .\auth_config.h
# Edit auth_config.h with your own Application (client) ID.
.\build_release.ps1
```

Expected owner build output: `Release/XCloud360.xex`. **No build, ZIP, Release modification or GitHub push was performed for this update.**

### Repository paths

- `src/xboxtls_backend.cpp`, `src/xbox_account.cpp`, `src/tls_trust.cpp`
- `third_party/xboxtls/` including original MIT license
- `auth_config.example.h`, `xex.xml`
- [TLS_ACCOUNT_INTEGRATION.md](TLS_ACCOUNT_INTEGRATION.md)
- [ROADMAP.md](../ROADMAP.md)

**XCloud360** is an independent research/homebrew project and is not affiliated with Microsoft, Xbox or Epic Games.
