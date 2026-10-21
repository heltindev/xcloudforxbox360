# Project root cleanup — 2026-10-20

**Platform:** native Xbox 360 PowerPC `.xex` on RGH3.  
**Goal:** fewer unnecessary files at the root, keeping every dependency required for the owner to build the native Xbox 360 project.

## What was moved (preserved, not deleted)

| Former location | New location | Reason |
| --- | --- | --- |
| `experimental_xell/` | `REFERENCE_BASE/XBOX360_EXPERIMENTOS/experimental_xell/` | Separate libxenon / XeLL boot-only experiment, **not** an XEX build dependency |
| `dist/` | `REFERENCE_BASE/ARQUIVOS_ANTIGOS/dist_antigo/` | Old, redundant XEX package and external font folder; preserved for historical reference |
| `XCloud360.vcxproj.user` | `REFERENCE_BASE/ARQUIVOS_ANTIGOS/XCloud360.vcxproj.user` | Per-user Visual Studio preference backup, not needed to build |
| `GR33N-main/`, `research_pkg/`, `research_tools/` | Previously moved to `REFERENCE_BASE/` | PS3 sources, DeepPlay media research, PKG inspector; not compiled into native Xbox 360 code |
| `XBOXTERMINAL/` | Its reusable TLS/login sources were previously consolidated into `src/` and `third_party/xboxtls/` | Duplicate standalone executable/project removed before this cleanup |

No PS4 source checkout was found in the project root. External PS4 references remain in `docs/REFERENCES.md`, without copying third-party code.

## What was removed (regenerable)

- `bin/`: prior Visual Studio `XCloud360.xdb` output.
- `obj/Xbox 360/`: old Xbox 360 object files, build logs, intermediate database and caches.
- `obj/retail/`: stale temporary converted XEX; future owner builds regenerate it if required.
- `obj/AtgFramework/Release/` **intermediate files only**: generated `.pch`, `.obj`, logs and related cache. **`AtgFramework.lib` was retained** because the main project links it.
- Root `XCLOUDFORXBOX360.sdf` and `XCLOUDFORXBOX360.suo`: user-specific Visual Studio IntelliSense/session caches, regenerated on demand.

**Approximate disposable cache space removed: 63.8 MiB (66,941,820 bytes).**

## What was deliberately kept

| Path | Why kept |
| --- | --- |
| `src/`, `third_party/`, `XCloud360.vcxproj`, `XCLOUDFORXBOX360.sln` | Main PowerPC Xbox 360 code and build configuration |
| `build_release.ps1`, `embed_font.ps1` | Owner-controlled compilation and embedded-font generation |
| `assets/Fonts/SegoeUI_24.xpr` | Local font source used by the build; **not published on GitHub** |
| `obj/generated/embedded_font_xpr.cpp` | Generated C++ font unit referenced by the XDK project |
| `obj/AtgFramework/Release/AtgFramework.lib` | Existing required static linker library |
| `tools/xextool/XexTool-windows-x64/XexTool.exe` | Local devkit-to-Retail XEX verification/conversion utility |
| `Release/` | User's current compiled XEX and other outputs; **not changed or cleaned** |
| `auth_config.h` | Locally configured Microsoft public application ID; **ignored by Git** |
| `REFERENCE_BASE/` | All research material and archived optional source; **ignored by Git** |
| `XCloud360-GitHub-Source/` | Reviewed, synchronized source-only mirror; no binaries/PS3 resources |

## Main directory after cleanup

```text
XCLOUDFORXBOX360/
  src/                         native Xbox 360 application and login adapters
  third_party/                 MIT-licensed source dependencies
  assets/Fonts/                local SDK font for XEX embedding
  obj/                         only essential generated font / ATG linker library
  tools/xextool/               locally installed XEX verification tool
  Release/                     current owner's build (preserved)
  docs/                        implementation status and update notes
  REFERENCE_BASE/             private PS3 references, XeLL experiment, archives
  XCloud360-GitHub-Source/     public-facing, source-only synchronization folder
  .vscode/                     developer-specific editor configuration
  build_release.ps1
  embed_font.ps1
  package_github.ps1
  README.md
  ROADMAP.md
  PROJECT_STATUS.md
  XCloud360.vcxproj
  XCLOUDFORXBOX360.sln
  xex.xml
  auth_config.example.h
  auth_config.h               private local file, ignored by Git
```

The `.git/` metadata directory remains at the root; it was not moved or modified.

## GitHub publication

Run `package_github.ps1` in the project root to refresh `XCloud360-GitHub-Source/`. It **only synchronizes and verifies source/documentation**; it does not compile, create a ZIP, push commits or modify `Release/`. The archived libxenon experiment is no longer exported and the mirror's old copy is removed **only after per-file hash verification**.

## Current functional limitations

**This is a filesystem and documentation cleanup, not a working xCloud update.** The integrated TLS/account code remains uncompiled after the recent changes. The real RGH previously experienced TCP error `10060`, while actual Microsoft login, WebRTC, streaming and Fortnite gameplay remain unverified or incomplete.
