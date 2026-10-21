# Build the paused project with Visual Studio 2010

This is a source package containing the solution, project, source files, tests, and scripts. It is not a Visual Studio 2010 or Xbox 360 XDK installer.

## Developer-installed dependencies

- Visual Studio 2010 C++ with Xbox 360 integration from an authorized SDK/XDK.
- `XEDK` pointing to the installation, available to both PowerShell and `devenv.exe`. Restart Visual Studio if you configure the variable after launching it.
- MSBuild 4.0 under `Windows/Microsoft.NET/Framework/v4.0.30319`.
- Windows PowerShell 5.1 (scripts use `PSScriptRoot`, `Get-FileHash`, and APIs from that generation).
- ATG project at `Source/Samples/Common/AtgFramework2010.vcxproj`.
- FontMaker or `bundler/SegoeUI_24.rdf` from the SDK samples.
- Optional for Retail builds: XexTool at `tools/xextool/XexTool-windows-x64/XexTool.exe`. Install it separately; it is not included.

You do not need to copy the original working folder or raw research archives. SDK files must come from your local installation. The scripts do not download SDKs or tools.

## First build

1. Copy the source directory to a local development location.
2. Set `XEDK` to the real path of your installation, not a path from another computer.
3. At the repository root, run `powershell -NoProfile -ExecutionPolicy Bypass -File .\prepare_vs2010.ps1`.
4. Open `XCLOUDFORXBOX360.sln`, select **Release | Xbox 360**, and press **F7**.

Preparation:
- creates `auth_config.h` from the example only when missing, without copying the author's credentials;
- generates the local font and `obj/generated/embedded_font_xpr.cpp`;
- builds ATG into `obj/AtgFramework/Release`;
- also runs automatically as a project pre-build step.

For Debug builds, add `-Configuration Debug`. Debug was not validated in this package; Release is the delivery configuration.

Standard VS2010 without Xbox 360 platform integration cannot compile this application. Do not switch the project to x86/x64 to attempt to generate an XEX.

## PowerShell build

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_release.ps1 -SdkOnly
```

This mode builds using the XDK without requiring XexTool; it does not inspect or convert the XEX to Retail. Output: `Release/XCloud360.xex`. For Retail inspection/conversion, separately install XexTool and run the script without `-SdkOnly`.

The project applies `xex.xml`. Intermediate outputs under `obj/` and `bin/` are not part of the published sources.

## Sign-in

The placeholder in `auth_config.example.h` allows compilation but blocks sign-in. Configure your own Microsoft application in local `auth_config.h` to investigate authentication. Xbox/xCloud permissions are controlled by Microsoft. Do not copy third-party application IDs to bypass authorization.

## Common errors

| Error | Check |
| --- | --- |
| Missing Xbox 360 platform | XDK/VS2010 integration is not installed |
| Missing XEDK | Process environment is not configured |
| Missing ATG | XDK Common samples |
| Missing font/bundler | XDK font tools and samples |
| Missing XexTool | Use `-SdkOnly` or install the tool separately |
| Sign-in not configured | Replace local placeholder with your own registration |
| TCP 10060 | Unresolved runtime issue; compilation alone will not fix it |
| No gameplay after sign-in | WebRTC and media playback are not integrated |

See `docs/VALIDATION_HANDOFF.md` for the clean-folder build result and `README.md` for remaining tasks.
