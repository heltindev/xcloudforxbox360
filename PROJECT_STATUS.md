# Project paused — 2026-10-21

The author has paused development and permits continued work through forks. xCloud is not playable yet. See README.md for the task list and docs/BUILD_VS2010.md for build instructions.

---

# Current status — 2026-10-21

**Experimental: does not play xCloud games yet.**

The repository root is the only active GitHub codebase. Read [AI_CONTEXT.md](AI_CONTEXT.md) and the [source reference summary](REFERENCE_BASE/AI_REFERENCE.md).

| Component | Status |
| --- | --- |
| UI, D3D9, local controls | Implemented |
| Device Code, Xbox User/XSTS, xCloud login | Code and simulated tests; no validated real authorization for this revision |
| Credentials and renewal | In-memory code, test-session use, and clearing |
| Session | Experimental creation, queue, connection, and termination; no continuous media |
| WebRTC / secure media transport | Not integrated |
| H.264 / Opus / presentation | Not integrated |
| Remote controls | Formatters, no active real channel |
| Console TCP 10060 | Fix not validated on hardware |

Host tests for authentication/HTTP, renewal/transfer, and session: passed. PowerPC compilation and Retail inspection: passed.

Local XEX: `Release/XCloud360.xex`.
SHA-256: 8D11D39B71D606B39CCA618B77884F7C2523FCB03BADE59C0BA9465E115634F2

Xenia, real accounts, and physical Xbox 360 were not tested in this revision. See [validation](docs/VALIDATION_CURRENT.md), [diagnostics](docs/XEX_GAPS.md), and [roadmap](ROADMAP.md). No commit, tag, or release was published as part of this reorganization.

## NET-10060-A1 network investigation

Added bounded IP acquisition waiting, DNS-address attempts, and X / NETWORK screen. Built with Visual Studio 2010 and verified as Retail. New simulated tests passed; resolution of the hardware timeout is still pending. [Testing instructions](docs/NETWORK_10060.md).

## Sign-in cache

Implemented a protected refresh-token cache, resumption before Device Code, rotation, and START-to-clear. Host tests passed; native persistence and protection still need console validation. [Details](docs/LOGIN_CACHE.md).
