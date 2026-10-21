# Changelog

## 2026-10-21 — Sign-in cache

- Protected refresh-token cache bound to application/scopes.
- Resumption before Device Code and persistence of token rotation.
- Transient errors and transfer/session failures preserve saved sign-in.
- START on Home removes the local cache.
- Persistent credentials excluded from Git and the publication checker.
- Host tests, VS2010 build, and Retail inspection passed. Native cache execution remains pending.

## 2026-10-21 — NET-10060-A1

- Waits up to 10 seconds for address acquisition within SYSAPP before networking attempts.
- HTTPS tries up to four distinct IPv4 addresses within a 12-second TCP wait budget.
- X / NETWORK screen shows IP, flags, DNS, errors, and HTTPS stage; Y runs network diagnostics from Home.
- Destination/attempt/timing logs without tokens. TLS and hostname validation remain enabled.
- Delayed DHCP, deadline, and rollover tests passed.
- Visual Studio 2010 compilation and Retail inspection passed.
- Console TCP 10060 remains pending physical verification.

## 2026-10-21 — Unified root and AI context

- Root becomes the only active repository; no second `src` tree is generated.
- Added `AI_CONTEXT.md`, curated summary, and hash index for targeted reading.
- Refresh and verification script updates the index without concatenating the entire codebase.
- Updated diagnosis: experimental session/refresh code exists, but transport and media remain unfinished.
- Original sources and historical artifacts excluded from initial context and Git.

## 2026-10-21 — In-memory credentials and HTTPS response handling

- gsToken and region are kept in the authentication worker, with at most 24 hours of validity and cleanup on expiry/cancellation.
- Cancellation between Xbox/XSTS/xCloud stages; intermediate tokens cleared.
- Duplicate sign-in is prevented while authenticated.
- HTTP responses with a known length or completed chunked encoding no longer depend on connection closure.
- Unsupported HTTP encodings and ambiguous headers are rejected.
- Reproducible public tests cover expiration, clock rollover, clearing, cancellation, regions, and HTTP fragmentation.
- No working gameplay session, WebRTC, or playback at this milestone. The hardware TCP 10060 error was not confirmed fixed.

## 2026-10-21 — Green interface and experimental authentication

### Changes
- Simplified green UI and controller navigation.
- Sign-in screen with official URL, device code, and remaining time.
- Corrected nested Xbox User/XSTS JSON response reading.
- Attempted xCloud service login after receiving a valid XSTS token.
- HTTPS, service login response, and region validation.
- HTTP failure and XErr diagnostics.
- README and roadmap aligned with the implementation status.

### Validation
- PowerPC Release build and Retail XEX inspection passed.
- Simulated authentication and HTTP construction tests passed.
- Latest visual revision not tested on Xenia or physical Xbox 360.
- Real Xbox/XSTS/xCloud sign-in not validated for this revision.

### Limitations
**This update does not enable xCloud gameplay.** Gameplay sessions, WebRTC transport, and video/audio decoding and playback are still missing. The intended design connects the Xbox 360 directly to the service without an intermediary PC during use.

The public sources use `auth_config.example.h`. Local account settings, SDK font files, proprietary tools, and binaries are not part of this source update.
