# Paused-project handoff validation — 2026-10-21

A source-only copy was created without the author's `auth_config.h`, `obj/`, `bin/`, `Release/`, generated fonts, or embedded SDK. Testing used development tools already installed on this machine, not a newly configured Windows installation.

| Check | Result |
| --- | --- |
| SDK build from clean copy using `build_release.ps1 -SdkOnly` | Passed |
| `auth_config.h` generated from example without author's account | Passed; placeholder ID blocks sign-in |
| Local font generation and ATG compilation | Passed |
| Visual Studio 2010 10.0.40219.1 solution, Release Xbox 360 | Passed; incremental build after clean build |
| Five MSVC 2010 host test suites | Passed |
| `ClCompile`/`ClInclude` references in packaged vcxproj | Checked; generated font is prepared at build time |
| Proprietary tools and SDK/XDK included in public folder | Not included |
| Clean-copy XEX converted/inspected as Retail | No; test used `-SdkOnly` |
| Debug configuration | Not tested |
| Real login, native cache, network, and console gameplay | Not validated |

The clean build produced compiler warnings but no fatal errors. `-SdkOnly` does not require XexTool. The VS2010 solution build also verified the automatic preparation target.

Suites cover simulated authentication/HTTP, refresh/transfer, simulated session lifecycle, simulated network status, and the Windows DPAPI cache. They do not connect to a real gameplay session.

Logs were kept only in the local working folder: `obj/handoff_build.log`, `obj/handoff_tests.log`, and `obj/handoff_vs2010.log`; they are not included in this package.

`GITHUB_MANIFEST.sha256` in the exported folder lists source/document hashes. It contains no secrets and does not certify streaming functionality.

Fork permissions and pending work are in `README.md`.
