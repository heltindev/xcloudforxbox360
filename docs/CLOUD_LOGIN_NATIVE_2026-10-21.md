# Native cloud service login checkpoint — 2026-10-21

Historical checkpoint: token retention and HTTP reading were subsequently updated; see [current credential lifecycle](CREDENTIALS_HTTP_2026-10-21.md) and README for the current binary hash.

This is authentication progress, NOT working game playback.

- The background worker now chains Microsoft OAuth -> Xbox User -> service-specific XSTS -> POST https://xgpuweb.gssv-play-prod.xboxlive.com/v2/login/user.
- Cloud login uses JSON containing the transient XSTS token and offeringId xgpuweb, with the fixed x-gssv-client: XboxComBrowser header, matching the locally inspected reference protocol.
- The HTTPS backend adds only this exact cloud hostname/path; TLS chain and hostname validation remain mandatory. No arbitrary endpoint or plaintext fallback was added.
- Valid cloud responses require gsToken, a positive durationInSeconds and at least one HTTPS Xbox service region. JSON string copies and response/token buffers are wiped. No cloud VM is allocated; the gsToken is not retained beyond this authentication checkpoint.
- XboxAccount publishes only non-secret result flags, region count and HTTP status. The minimal green UI distinguishes cloud login pending/confirmed and displays failure diagnostics.
- Private auth_config.h now requests XCLOUD360_ENABLE_XBOX_LIVE_SCOPE=1 with the existing client ID. This requests consent; it does not grant permissions or bypass Microsoft. The previous private config is backed up under obj/auth_config.before_cloud_scope.h. No client ID was replaced.

Validation:

- MSVC 2010 host regression tests: PASS. Covers three-stage mock login, HTTP framing, unverified TLS, cloud 403, invalid region, nested JSON and malformed token replies.
- Xbox 360 PowerPC Release compilation: PASS.
- Retail XEX verification: PASS.
- Native startup includes the exchange regression tests; those tests were compiled but not executed in Xenia or physical Xbox during this revision.
- Real Microsoft consent, Xbox User/XSTS and cloud login: NOT TESTED in this revision.
- xCloud session creation, WebRTC, H.264 decoding, Opus playback, gameplay: NOT IMPLEMENTED.

Output: Release/XCloud360.xex
SHA256: 7BE20CF855695E5BC18B8DE7B0371D189296B45AF77DFEE67300E685650503FF

Runtime remains console-direct, without a PC relay. A phone/browser is used only for Microsoft device-code approval. Test results do not establish physical Xbox 360 compatibility or streaming capability.
