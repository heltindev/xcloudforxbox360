# Fortnite single-button XEX — implementation and test log

**Date:** 2026-10-20  
**Target platform:** native Xbox 360 Xenon PowerPC, intended for RGH3.  
**Currently tested platform:** Xenia Canary on the development PC, **not a real Xbox 360**.

## Request

Enable a game selection screen for Fortnite at:

`https://play.xbox.com/products/BT5P2X999VH2/fortnite`

Make controller **A** start every presently available setup step; label logs **1, 2, 3, 4, ...** and show honest percentages. Give people sufficient time to enter a Microsoft Device Code in a phone/PC without repeatedly starting the login attempt.

## Code changes

- `src/app.h`, `src/app.cpp`: native menu with one game entry, `1 > FORTNITE [SELECTED]`, visible target URL, A-start workflow, progress bar and numbered steps.
- Step 1: select Fortnite (10%). Step 2: local cryptographic/protocol and JSON tests (20%). Step 3: DNS/TCP tests (up to 35%). Step 4: request Microsoft Device Code over authenticated HTTPS (45%). Step 5: real code displayed in a separate region with expiry countdown (60%). Step 6: Microsoft Device Code flow confirmation if server approves (70%).
- Step 7: **BLOCKED**, not a fabricated 100%. The project has no WebView/HTML5/JavaScript browser, permissioned xCloud session, negotiated WebRTC stream, H.264/Opus decoder or input data channel. Showing a URL is not playing Fortnite.
- `src/xbox_device_flow.cpp`: preserve server-defined expiration, rebase expiry after the HTTPS device-code response, defer the *first* token poll for 30 seconds (or longer if the server's interval requires it), and follow server polling/backoff afterward.
- All user codes appear **only on screen**, never in telemetry/debug logs. Tokens are securely erased when the local flow exits; they are **not** Xbox Live/XSTS credentials.
- A second A press does **not** reset an active code. B cancels a running attempt; the next A can retry when the worker is free.

## Important Microsoft login distinction

A legitimate HTTP 400 during OAuth token polling may contain `authorization_pending` or `slow_down`: these keep the device code valid and should not end the workflow. Other HTTP 400 codes, including client permission errors, must be reported rather than ignored. The existing error parser preserves the safe OAuth error name, without logging Microsoft tokens, secrets or error descriptions containing personal information.

An issued Microsoft Device Code **expires at the time the server says it does**, often many minutes; extending it artificially is not possible. The 30-second initial poll grace period ensures the first validation attempt is not made immediately after showing the code, but the code still requires the user's approval at the official Microsoft URL.

## Observed Xenia results

| Validation | Observation |
| --- | --- |
| Latest PowerPC compilation | **PASS** with Xbox 360 XDK |
| Retail XEX conversion | **PASS** with XexTool |
| XEX/menu boots in Xenia | **PASS** |
| Native Fortnite entry, full target URL and progress UI | **PASS** on captured screenshot `xenia_fortnite_one_button_menu.png` |
| Offline protocol and JSON checks | **PASS** in latest Xenia screenshot |
| Microsoft HTTPS Device Code | **PASS** in earlier native build after fixing TLS 1.3 key derivation; a genuine Microsoft Device Code response appeared in the UI |
| Actual new A-button flow to Device Code on Xenia | **NOT CONFIRMED BY AUTOMATION**; use a real gamepad and send an updated Xenia screenshot |
| Account approved and Microsoft token received | **NOT CONFIRMED** |
| Actual xCloud/Fortnite gameplay | **NOT IMPLEMENTED** |
| Latest XEX on physical Xbox 360 RGH3 | **NOT TESTED** |

A temporary Xenia keyboard-input attempt could not be validated reliably and is **not counted as a passed controller test**. The development PC's Xenia settings were not permanently changed. Input should be verified by the owner with an actual controller while the Xenia window is active.

## Next owner's test

1. Launch `Release/XCloud360.xex` in Xenia.
2. Connect a controller recognized by Xenia, highlight `1 > FORTNITE`, and press **A** once.
3. Wait for numbered steps 1–4. The DNS fallback result is not equivalent to TLS verification.
4. When `5. [60%]` appears, open the **real URL shown by Microsoft** using a phone or PC and enter the code privately. Check that the code remains visible for the server-defined validity period.
5. If the flow fails, share only the numbered step, the safe server error name, HTTP status, TLS error and expiry observation. **Never post passwords, access tokens, device codes or refresh tokens.**
6. Independently repeat on the physical RGH3 and note any TCP 10060 timeout.

**No ZIP files, no Github publish, no external game runtime, no invented WebRTC stream.** Private backups, screenshots and test scripts remain in `REFERENCE_BASE/ARQUIVOS_ANTIGOS/`, excluded from the source mirror.
