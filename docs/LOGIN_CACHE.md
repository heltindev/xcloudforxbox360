# Sign-in cache — 2026-10-21

## Usage

1. Install the new `Release/XCloud360.xex` in a writable folder on Xbox 360.
2. Sign in to Microsoft using this version. Previous versions erased tokens, so their sessions cannot be restored.
3. The app attempts to save an encrypted refresh token. Upon returning to Home, check for **Sign-in saved on this console**.
4. Close and reopen the app, then press A. It attempts to renew the saved account before requesting another code.
5. To remove the saved account or switch accounts, press **START — CLEAR SIGN-IN** on Home.

The cache is stored alongside the executable at `game:\XCloud360.authcache`. Keep it in the same folder and on the same console when replacing the XEX. Do not delete it during updates if you want to retain sign-in.

## Behavior

- Stores only refresh token, application/scopes, and previously received permissions. No password, device code, Xbox User token, XSTS token, or gsToken is stored on disk.
- Xbox uses `XeKeysObfuscate` / `XeKeysUnObfuscate` with console-specific protection. The file is opaque, with no plaintext fallback.
- Protection is round-trip tested in memory before saving.
- Windows tests use Windows DPAPI, not Xbox protection.
- The protected record binds the client ID and requested scopes; configuration changes require another sign-in.
- Rotated refresh tokens are saved after successful login, refresh, or token transfer.
- Transient network/service failures preserve the cache and do not automatically trigger another Device Code flow.
- A verified normal refresh response requiring interactive authentication can cause the cache to be removed and another code requested.
- A transfer-authorization or session failure does not, by itself, invalidate the Microsoft login.
- START cancels the current flow and deletes the cache after the worker finishes. The UI does not block while waiting for an HTTPS call.
- Writes use a protected temporary file, flush, and replacement. Write errors preserve the previous file and are displayed in the UI.
- Tokens in RAM and temporary buffers are wiped when objects are released.
- Deleting the local cache ends the local saved session but does not globally revoke the Microsoft account.

## Limitations

Resuming still requires internet access, valid tokens, consent, and Microsoft-granted permissions. Caching neither fixes TCP 10060 nor provides xCloud media.

If protection or storage fails on the console, sign-in may work without being saved; the Home screen will show a cache error. Xenia support depends on its emulation of the required native exports and is unvalidated.

Do not publish `*.authcache*` files or send them to anyone. Git and the publication checker exclude them.

## Implementation

| File | Role |
| --- | --- |
| `src/login_cache.h/.cpp` | Bounded record, validation, binding, and clearing |
| `src/login_cache_platform.h/.cpp` | Native/DPAPI protection and protected file |
| `src/xbox_device_flow.cpp` | Refresh resumption, rotation, and persistence callback |
| `src/xbox_account.cpp` | Loading, selective invalidation, and coordinated deletion |
| `src/app.cpp` | Cache status and START-to-clear |
| `tests/login_cache_tests.cpp` | Actual host DPAPI protection and storage failure tests |
| `tests/oauth_renewal_tests.cpp` | Device Code-free resumption and simulated error handling |

## Verification

Host tests for restoration, rotation, application/scope binding, tampering, truncation, write failures, protection refusal, removal, temporary failures, and verified authentication errors passed.

The Visual Studio 2010 / PowerPC build and Retail inspection passed. Native protection, restoration with a real account, and persistence on Xbox 360/Xenia were not executed in this revision.

XEX SHA-256: **A41E62588D4E7BE41E3BDB18F77B626131886E626CE68195A8413DAF44D408B4**

Renewal flow reference: https://learn.microsoft.com/en-us/entra/identity-platform/refresh-tokens
