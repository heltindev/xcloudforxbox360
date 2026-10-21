# XCloud360 — Development paused

**I have paused XCloud360 development. Anyone interested in continuing can fork this repository.** I am making the source code and available information public for anyone who wants to investigate, fix issues, and develop the next stages.

**This project DOES NOT yet play Xbox Cloud Gaming titles.** There is no complete gameplay session with working video, audio, and controls on Xbox 360. Successfully building a `.xex` does not mean streaming works.

The original goal is a native client for RGH/JTAG Xbox 360 consoles, with no intermediary PC during streaming. A PC is required for development and compilation; approval of Microsoft sign-in uses a browser on another device.

## What is implemented

- Native C++ PowerPC/big-endian application, Visual Studio 2010 solution, and a green D3D9 interface.
- Controller navigation, DNS/TCP probes, and experimental HTTPS using XboxTLS.
- Experimental Microsoft Device Code, Xbox User, XSTS, and xCloud login code.
- Protected refresh-token cache with restoration and START-button deletion. Native console persistence has not yet been validated.
- Experimental session creation, queue polling, connection, and termination code.
- HTTP/JSON/RTP parsers and formatters, H.264 reassembly, and control messages.
- Host tests for simulated authentication, token renewal, sessions, networking, and caching.
- `AUTH-DIAG-A2` diagnostics screen, opened with X.

**Current limitation:** the session is stopped after the test. There is no integrated player. `src/webrtc_xbox.h` contains declarations, not a functional WebRTC peer implementation.

## Known issues and work needed

- [ ] **Validate networking on Xbox 360:** investigate TCP timeout `10060`; record addresses, DNS, connect, TLS, and HTTP read results. The root cause is not confirmed.
- [ ] **Validate real sign-in:** confirm the application's permissions, Microsoft consent, Xbox User, XSTS, and xCloud access for this revision.
- [ ] **Validate the cache on console:** verify native protection, writing, restoration after restart, rotation, and deletion. Windows tests use DPAPI and do not prove Xbox protection.
- [ ] **Validate a real session:** request a game, track the queue, and confirm connection and cleanup. Current tests use simulated responses.
- [ ] **Keep sessions running:** replace test-only teardown with continuous execution once a media pipeline exists, including credential renewal and cancellation.
- [ ] **Implement signaling:** exchange SDP, ICE candidates, and media configuration according to the service protocol.
- [ ] **Integrate WebRTC:** implement or port ICE, STUN/TURN, DTLS, SRTP/SRTCP, and SCTP/DataChannels.
- [ ] **Receive and organize media:** packet ordering, jitter buffers, RTCP, loss recovery, and memory bounds.
- [ ] **Decode H.264:** choose and port a compatible decoder. No hardware-decoder API has been confirmed for this codebase.
- [ ] **Display video:** render decoded frames using D3D9 textures.
- [ ] **Decode Opus:** feed XAudio2 and synchronize audio/video.
- [ ] **Send controls:** connect the existing message formatters to a real remote channel.
- [ ] **Handle errors:** reconnect, cancel, expire credentials, and release resources safely.
- [ ] **Test on hardware:** measure memory, performance, latency, stability, and differences from Xenia.
- [ ] **Validate complete gameplay:** video, audio, and controls directly on the Xbox 360.

**Suggested order:** network/sign-in/cache → session → signaling/WebRTC → media/controls → hardware testing.

The project owner reported that earlier attempts did not work. There is insufficient evidence to mark network connectivity, full login, or streaming as validated in this revision. See the [current diagnosis](docs/CURRENT_DIAGNOSIS.md), [technical blockers](docs/XEX_GAPS.md), and [validation status](docs/VALIDATION_CURRENT.md).

## Build with Visual Studio 2010

This package contains the active sources, required source dependencies, tests, scripts, solution, and documentation. **Visual Studio 2010 alone is not enough:** separately install a compatible, authorized Xbox 360 SDK/XDK, including VS2010 Xbox 360 integration, the PowerPC compiler, libraries, and ATG samples.

Local requirements:

- Windows with Visual Studio 2010 C++ and the **Xbox 360** platform installed by the XDK.
- `XEDK` environment variable pointing to the XDK installation.
- MSBuild 4.0 and Windows PowerShell 5.1.
- `Source/Samples/Common/AtgFramework2010.vcxproj` and font tools from XDK samples.
- For Retail conversion/inspection: install XexTool separately at `tools/xextool/XexTool-windows-x64/XexTool.exe`.

The SDK/XDK, Microsoft libraries, proprietary tools, SDK-generated fonts, personal settings, tokens, and Git history are not included. The build generates SDK resources from the local installation.

### Build from Visual Studio 2010

1. Open PowerShell with `XEDK` configured and go to the project folder.
2. Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\prepare_vs2010.ps1
```

3. Open `XCLOUDFORXBOX360.sln` in Visual Studio 2010.
4. Select **Release | Xbox 360** and choose **Build Solution / F7**.

Preparation creates `auth_config.h` from the example only if the file does not exist, generates the font, and builds ATG locally. The project also runs this preparation before builds. The example client ID is blank/intentionally invalid: it allows interface compilation but **does not enable sign-in**.

### Build from PowerShell

SDK-only build without XexTool:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_release.ps1 -SdkOnly
```

Build with Retail inspection and conversion, after installing XexTool:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_release.ps1
```

Output: `Release/XCloud360.xex`. The `-SdkOnly` option does not guarantee Retail format or operation on RGH. See the [VS2010 build guide](docs/BUILD_VS2010.md).

### Sign-in configuration

Edit only your local `auth_config.h`, using the application/client ID of a Microsoft application you control that supports Device Code. The example keeps `XCLOUD360_ENABLE_XBOX_LIVE_SCOPE=0`. Requesting `xboxlive.signin` requires appropriate permissions; changing the flag does not guarantee Xbox/xCloud authorization. Do not use third-party app IDs to bypass permissions.

### Windows tests

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\run_host_tests.ps1
```

These tests do not launch games or prove connectivity to the service. Windows cache tests use DPAPI; the Xbox uses a different implementation.

## Repository layout

| Path | Contents |
| --- | --- |
| `XCLOUDFORXBOX360.sln`, `XCloud360.vcxproj`, `xex.xml` | VS2010 solution, Xbox 360 build, and XEX configuration |
| `src/` | Active application sources |
| `third_party/` | Available source dependencies and notices |
| `tests/` | Host tests |
| `docs/` | Diagnostics, limitations, and historical records |
| `REFERENCE_BASE/` | Compact AI reference summary and source map only |
| `prepare_vs2010.ps1`, `build_release.ps1` | Preparation and compilation |
| `AI_CONTEXT.md` | Short technical context |

The original bulk research files are not needed to build and are not included. Older documents describe their respective historical revisions.

## For contributors and forks

You may continue development, fix issues, and publish your progress in a fork. Preserve the credits and notices/licenses of third-party components. This permission applies only to my contributions; it does not change rights to dependencies or Microsoft materials. See [third-party notices](THIRD_PARTY_NOTICES.md).

I recommend starting with a reproducible networking test on a physical Xbox 360. Record the source revision, error, and result for every step. Do not claim playable support until an entire gameplay session is verified.

Thank you to everyone interested in carrying on this research and helping keep the Xbox 360 alive. 🎮

Independent project, not affiliated with Microsoft, Xbox, or Epic Games.
