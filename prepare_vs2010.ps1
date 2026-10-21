param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$msbuild = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\MSBuild.exe'
if ([string]::IsNullOrWhiteSpace($env:XEDK)) {
    throw 'Set XEDK to your locally installed, authorized Xbox 360 SDK/XDK.'
}
$atgProject = Join-Path $env:XEDK 'Source\Samples\Common\AtgFramework2010.vcxproj'
foreach ($required in @($msbuild,$atgProject,(Join-Path $root 'auth_config.example.h'))) {
    if (!(Test-Path -LiteralPath $required -PathType Leaf)) { throw "Required build file missing: $required" }
}
$config = Join-Path $root 'auth_config.h'
if (!(Test-Path -LiteralPath $config)) {
    Copy-Item -LiteralPath (Join-Path $root 'auth_config.example.h') -Destination $config
    Write-Warning 'Created local auth_config.h with a placeholder ID. Configure your own app for sign-in.'
}
& (Join-Path $root 'embed_font.ps1')
$atgDir = Join-Path $root ('obj\AtgFramework\' + $Configuration)
$atgLibrary = Join-Path $atgDir 'AtgFramework.lib'
if (!(Test-Path -LiteralPath $atgLibrary)) {
    New-Item -ItemType Directory -Force -Path $atgDir | Out-Null
    $atgOutput = $atgDir + '\'
    & $msbuild $atgProject /t:Build (('/p:Configuration={0};Platform=Xbox 360' -f $Configuration)) ('/p:OutDir=' + $atgOutput) ('/p:IntDir=' + $atgOutput) /v:minimal
    if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $atgLibrary)) { throw 'Local ATG framework build failed.' }
}
Write-Host ('VS2010 preparation ready: ' + $Configuration + ' | Xbox 360')
