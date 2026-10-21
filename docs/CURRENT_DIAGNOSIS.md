# AUTH-DIAG-A2 diagnostics

The user reported that the previous revision did not work. Without new console results, the cause remains undetermined. This revision improves observability; it does not implement xCloud playback.

## Console test

1. Copy `Release/XCloud360.xex` into a writable folder. Preserve `XCloud360.authcache` if present; do not share this file.
2. Launch the XEX, press X, and confirm the `AUTH-DIAG-A2` title. From startup, Y launches the network test.
3. Let the test finish and photograph the entire screen.
4. Go back and press A to sign in. After any failure, press X and take another picture. Never photograph/share sign-in codes, QR codes, tokens, or passwords.
5. Report whether the test used a physical Xbox 360 or Xenia and whether the failure was launching the XEX, requesting a code, granting authorization, restoring sign-in, or starting a session.

## Interpreting results

- **WSA/TLS/HTTP:** last HTTPS stage. HTTP 0 means no recorded HTTP response, not success.
- **XBOX / XSTS / CLOUD:** HTTP and OK values for each exchange. HTTP 0 can mean the stage was not reached.
- **XERR:** numeric error returned by Xbox authorization.
- **CACHE:** 0 operation completed; 1 missing file; 2 file error; 3 invalid record; 4 different application/scopes; 5 protection failure. Code 0 alone does not prove restoration; check saved/restored status.
- **SESSION:** internal stage, HTTP status and cleanup HTTP status. This test still stops the session and does not play media.
- **TCP probes:** 0 pending; 1 HTTP response; 2 failure; 3 preliminary TCP result. Origin 8 means TCP wait.

## Verification for this revision

`build_release.ps1` compiled with the XDK/MSVC 2010 toolchain and passed Retail inspection. Local log: `obj/build_auth_diag_a2.log`. Only presentation of existing diagnostics changed; host suites from the cache revision were not rerun. Layout and behavior on physical hardware were not validated.

SHA-256: A41E62588D4E7BE41E3BDB18F77B626131886E626CE68195A8413DAF44D408B4
