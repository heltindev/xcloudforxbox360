# XCloud360 terminal — test report (2026-10-20)

## What changed

- Backed up the former graphical Games/Favourites/DeepPlay/GR33N `src/app.cpp`, `src/app.h`, project XML and docs into the ignored `PASTACODIGOBASE/ARQUIVOS_ANTIGOS/DASHBOARD_GR33N_20261020/` folder with SHA-256 verification.
- Replaced `src/app.cpp` and `src/app.h` with a compact Direct3D-rendered **text terminal** for Xbox 360 Xenon PowerPC. It has no game grid, covers, dashboard tabs, web engine, JavaScript, or PC runtime dependency.
- On startup, executes local protocol self-tests and regional JSON tests. Controller **Y** reruns them, **X** starts asynchronous DNS/TCP checks, **A** attempts Microsoft Device Code login, **B** cancels sign-in and **BACK** exits.
- Corrected a Visual Studio object-file name collision: the XCloud360 renderer and MIT XboxTLS13 socket layer both used `platform_xbox360.cpp` and generated the same object filename. The TLS file now uses `$(IntDir)xboxtls_platform_xbox360.obj`.
- Corrected a WinSock initialization cleanup call to use the Xbox 360 XDK's `NetDll_XNetCleanup(XNCALLER_SYSAPP)`.
- Corrected network status wording. **A successful direct-IP connection is not displayed as Xbox.com or xCloud connectivity**.
- The current `Release/XCloud360.xex` is newly generated; before this work `Release/` was missing. The legacy UI source was preserved privately instead of deleted irreversibly.

## Concrete compilation results

**Native Xbox 360 XDK build executed on the authorized development PC.**

1. First attempt: linker error `LNK4042`, nine unresolved symbols caused by two `platform_xbox360.obj` files. No valid XEX.
2. Second attempt: SDK C2064 from incorrect `XNetCleanup()` call in the TLS source. Corrected.
3. Third attempt: **success** (MSBuild exit 0), XexTool converted and verified compressed Retail XEX.
4. Final attempt, after clarifying the IP-only network status: **success** (MSBuild exit 0).

Final file: `Release/XCloud360.xex`.

Final SHA-256 at build time: `C03D5766DA25AE801512E6B12C7D166ED509F3466D4FC3039E8C6B8034C7F63E`.

No ZIP, GitHub upload, or installation onto the owner's physical Xbox 360 was performed.

## Actual Xenia smoke-test output

Launched the actual generated PowerPC XEX using local Xenia Canary. The process remained running and displayed the new terminal, with these observed values (a private screenshot and logs were saved in the backup folder):

| Probe | Observed |
| --- | --- |
| XEX launch and terminal font | **PASS** |
| Offline protocol self-test | **PASS** |
| Offline regional JSON self-test | **PASS** |
| Direct IPv4 TCP `1.1.1.1:443` | Connected according to probe |
| Direct IPv4 TCP `1.1.1.1:80` | Connected according to probe |
| `www.xbox.com` TCP 443 | **FAIL, 10060** |
| Xbox DNS records for `www.xbox.com` | **0** in this emulator test |
| Microsoft Device Code login attempt | **FAILED**, stage `DNS RESULT`, TLS adapter result `-8`, underlying net error `1`, HTTP `0` |
| Actual authenticated Microsoft response / code | **NONE** |
| Actual streaming from `xbox.com/play` | **NOT ATTEMPTABLE**, browser/WebRTC/decoder not implemented |

The screenshot included controller events for A (device-login attempt), X (network check) and Y (offline re-test). The real RGH3 was **not accessible through these computer-side tools**. Do not extrapolate these Xenia network results to physical RGH.

## Interpretation

The Xbox 360 has no automatically embeddable HTML5 browser/WebView through Xbox XDK, and a URL by itself does not provide the HTML/JavaScript/Media Source Extensions/WebRTC stack needed by `xbox.com/play`.

The code currently implements a native probe, not the website. There are also major native streaming implementation tasks (Xbox Live/XSTS authorization, service session, WebRTC/DTLS/SRTP/UDP, video and audio codecs, latency and input). The historical physical-console `10060` problem is still unresolved until rerun on real RGH.

An existing local public application ID is configured for experiments but was **not displayed or exported**. Account tokens were not observed in the Xenia test.

## Next owner hardware test

1. Copy **only** `Release/XCloud360.xex` to a folder on the Xbox 360 RGH3 and launch with Aurora or XeXMenu.
2. Confirm the text terminal and `SELFTEST: PASS` appear. Run **Y** again.
3. Press **X**; report `XBOX443`, `IP443`, `IP80`, `SOURCE`, `DNS_IP`, the port and the exact error code.
4. Press **A** to attempt login only if willing to test the experimental TLS client; note `TLS`, `NETERR`, `HTTP` and stage. **Do not enter your Microsoft password into the XEX.**
5. If TCP 10060 persists, focus on the retail XNet network path and security limitations; do not treat changing DNS or opening ports as a guaranteed fix.

Future milestones require real HTTPS and authorization plus implementation of a browser or a dedicated native WebRTC streaming client. This build **does not play Fortnite**.
