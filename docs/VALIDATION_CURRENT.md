# Current validation — sign-in cache — 2026-10-21

| Check | Result |
| --- | --- |
| Simulated authentication/HTTP | Passed |
| Refresh, transfer, Device Code-free resumption, and selective invalidation | Passed |
| Simulated session: queue, connect, cleanup | Passed |
| Simulated DHCP/timeout/rollover | Passed |
| Protected Windows DPAPI cache, rotation, tampering, and write failures | Passed |
| Visual Studio 2010 10.0.40219.1, Release Xbox 360 | Passed |
| XEX Retail inspection | Passed |
| Native cache/protection on Xbox 360 and Xenia | Not tested in this revision |
| Real login/session for this revision | Not validated |
| Video, audio, and controller playback | Not implemented |

Local artifact: `Release/XCloud360.xex`.
SHA-256: **A41E62588D4E7BE41E3BDB18F77B626131886E626CE68195A8413DAF44D408B4**

Windows cache tests use DPAPI and do not prove `XeKeysObfuscate` works on console. Other tests use synthetic responses. Real restoration still depends on networking and Microsoft authorization.

Local logs: `obj/test_login_cache_final.log`, `obj/vs2010_cache_final.log`, `obj/build_cache_retail.log`.

See [sign-in cache](LOGIN_CACHE.md) and [network diagnostics](NETWORK_10060.md). The user reported the previous attempt failed; without new network-screen values, its root cause remains unknown.

**AUTH-DIAG-A2:** X shows separate cache, Xbox User, XSTS, xCloud, and session results. Build/Retail inspection passed; hardware layout and behavior were not validated. The previous revision's host suites were not rerun after the UI-only change. See `docs/CURRENT_DIAGNOSIS.md`.

## Paused handoff

The author paused the project and permits forks to continue their own contribution. The clean-copy build, tests, and delivery limitations are documented in [VALIDATION_HANDOFF.md](VALIDATION_HANDOFF.md).
