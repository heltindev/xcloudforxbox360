# Credentials and HTTPS response handling — 2026-10-21

This revision prepares sign-in for a future gameplay session. **It does not implement streaming.**

## Changes

- `CloudCredentials` keeps the gsToken and regional hostname only on the authentication thread, with no copies in the UI, logs, or on disk.
- A valid default region takes priority; otherwise the first valid Xbox HTTPS region is selected.
- Expiration uses a monotonic 32-bit clock, including rollover. Retention is limited by the service-supplied expiration and capped at 24 hours. There is no automatic renewal.
- Cancellation, expiration, failed new exchange, or owner destruction clears the credential.
- Cancellation is checked before each following Xbox/XSTS/xCloud request and before accepting the token. An HTTPS call already in progress still depends on its own timeouts.
- Microsoft/device credentials are cleared after exchange; only the gsToken remains while the screen is authenticated.
- The UI avoids starting another sign-in while the current credential is retained.
- HTTPS reading ends as soon as the full Content-Length body or complete chunked response arrives, without waiting for EOF. Unsupported encodings, duplicate transfer headers, and ambiguous combinations are rejected.

The chunked reader still does not support extensions or trailers. Responses without explicit length still rely on EOF.

## Reproducible verification

Run on Windows using Visual Studio 2010:

```powershell
.\tests\run_host_tests.ps1
```

The `-VcVars` parameter can specify another compatible compiler environment. Tests use simulated responses without an account or network.

Coverage includes the Xbox/XSTS/xCloud chain, region preference, exact expiration, clock rollover, credential clearing, cancellation at every exchange boundary, TLS/HTTP failures, invalid fractional expiration, fragmented responses, chunked encoding, and ambiguous HTTP headers.

## Outstanding real-world work

| Stage | Status |
| --- | --- |
| Real consent and sign-in | Not validated in this revision |
| TCP 10060 on hardware | Not yet diagnosed with this revision |
| In-memory credentials | Implemented; at this historical stage, no session consumer |
| Create game, queue, connect, terminate | Not implemented at this historical stage |
| WebRTC / ICE / DTLS / SRTP / SCTP | No integrated transport |
| H.264 video / Opus audio | No integrated decoding or playback |
| Remote controls | No real channel connected |
| Xenia and physical Xbox 360 | This revision was not run |

The HTTP read fix takes effect only after establishing TLS and receiving a response. It does not automatically fix an initial TCP connection timeout. Diagnose that blocker by recording stage, WSA/TLS error, network flags, and DNS results on the console, without exposing credentials.

This revision does not allocate a game machine. The next module must consume credentials on the same thread, check validity before each request, handle queue states, and terminate sessions on cancellation. WebRTC negotiation, decoders, and real tests are still required afterward.

## Build result for this revision

Simulated MSVC 2010 tests: passed. PowerPC Release build and Retail inspection: passed. Not executed on Xenia or a physical Xbox 360.

SHA-256: 4A51BAFC00AA898709314051B659B9F897562DDF6E3B27F438602F23E5B85E36
