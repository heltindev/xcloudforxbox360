# Xbox 360 source consolidation — 2026-10-20

**Later cleanup:** The project root was simplified further by archiving `experimental_xell/` and redundant `dist/` outputs under `PASTACODIGOBASE/` and deleting only regenerable build/IDE caches. See [ROOT_CLEANUP_2026-10-20.md](ROOT_CLEANUP_2026-10-20.md). The native XEX source, required linker libraries and `Release/` remain intact.

**Target:** Xbox 360 RGH3, Xenon PowerPC, **native XEX without a PC intermediary**.

This update reorganizes the source without building or replacing the executable. It is intended for the GitHub source mirror and for future Xbox 360 RGH tests.

## 1. Single Xbox 360 application

The former standalone **`XBOXTERMINAL/`** TLS proof of concept has been removed, after migrating the relevant, MIT-licensed source and checking its hashes. The XboxTLS13 library now belongs to the main project under `third_party/xboxtls/`.

The Xbox 360 source uses `src/xboxtls_backend.cpp`, `src/xbox_account.cpp`, `src/tls_trust.cpp`, `src/secure_http.cpp` and `src/xbox_device_flow.cpp` to **attempt** trusted HTTPS and Microsoft device-code authentication in the unified UI. The manifest `xex.xml` is part of the main XDK project.

**Status:** source integrated into the project, **not yet compiled or successfully tested on real RGH**. No valid xCloud session or playable Fortnite exists.

## 2. Research code isolated in PASTACODIGOBASE

All **locally available** PS3 research materials were moved from the project root to the private reference folder:

```text
XCLOUDFORXBOX360/
  PASTACODIGOBASE/              (ignored by Git, never required by XEX)
    GR33N-main/                (108 original source/reference files)
      XCLOUDPS3.pkg            (DeepPlay PS3 package; not GR33N executable)
      source/ include/ deps/   (PS3 source, GPL-3.0)
    research_pkg/              (8 privately extracted PKG previews/resources)
    research_tools/            (PS3 PKG offline inspector)
    README.md
```

`GR33N-main` was initially locked against a direct Windows move. The source was copied, **all 108 file hashes were verified with SHA-256**, and the old original was deleted only after verification. The two smaller research folders moved directly. The moved PKG inspector was executed again from its new location and returned the expected **55 package entries**.

**No local PS4 source checkout existed** during the inventory. The PS4 projects remain external research references in `REFERENCES.md`; no third-party PS4 source was downloaded or silently included.

The root `.gitignore` excludes `PASTACODIGOBASE/`. It contains PS3-specific code and PS3 binary media; it cannot be linked directly by the Xbox 360 SDK and must not be bundled into a GitHub release.

## 3. Source mirror for GitHub

`XCloud360-GitHub-Source/` contains the current first-party Xbox 360 project sources and the portable dependencies required to reproduce the build (subject to separately installed licensed XDK/SDK tools):

- `src/`, `third_party/xboxtls/`, `third_party/cjson/`
- `auth_config.example.h`, `xex.xml`, `XCloud360.vcxproj`, `XCLOUDFORXBOX360.sln`
- `build_release.ps1`, `embed_font.ps1`, documentation and the source-mirror synchronizer.

The source mirror **excludes** `PASTACODIGOBASE/`, `auth_config.h`, Xbox SDK assets, compiled XEX/EXE, proprietary PS3 media, XexTool binaries and other private artifacts.

`package_github.ps1` now **synchronizes the existing source mirror rather than creating ZIP files**. No upload/push, compilation, ZIP, or `Release/` modification was made.

## 4. What has been achieved vs. what is still missing

| Component | Current actual status |
| --- | --- |
| One native XEX source project + integrated TLS/OAuth modules | **Source organized; uncompiled** |
| Original GR33N/DeepPlay reference code preserved | **Moved; all copied GR33N hashes verified** |
| Local DeepPlay PKG inspector | **Tested after move** |
| GitHub-facing source mirror / licensing files | **Synchronized; no secret configuration copied** |
| DNS on real Xbox 360 RGH | Previously **one IPv4 address resolved** |
| Direct TCP to Xbox.com / 1.1.1.1 on RGH | Previously **10060 timeout**; remains unresolved |
| Real TLS connection / secure Microsoft device login | **Not demonstrated**, needs compilation and RGH network test |
| Xbox Live/XSTS, xCloud session authorization, WebRTC, video/audio decoding, playable Fortnite | **Not yet available** |

### Next step

The project owner compiles the latest source on the Xbox 360 SDK toolchain and tests it on a physical RGH. Record the **first compiler error** (if any) and the native TCP/TLS diagnostic details. Updating DNS alone is not a verified solution to previous TCP timeouts.

Related: [TLS_ACCOUNT_INTEGRATION.md](TLS_ACCOUNT_INTEGRATION.md), [ROADMAP.md](../ROADMAP.md), [RELEASE_NOTES_UNIFIED_TLS.md](RELEASE_NOTES_UNIFIED_TLS.md).

> Xbox Cloud Gaming, Xbox, Microsoft, PlayStation, GR33N, XCloud4 and DeepPlay are separate trademarks/products. Source and binary license requirements must be respected.
