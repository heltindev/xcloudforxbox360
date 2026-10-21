# XCloud360 build for native Xbox 360 PowerPC, launched by the owner.
# Intended output: Release/XCloud360.xex (Retail, with embedded font).
# No ZIP, no dist folder and no XCloud360-RGH3 folder are created.
param([switch]$SdkOnly)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$project = Join-Path $root 'XCloud360.vcxproj'
$msbuild = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\MSBuild.exe'
$xextool = Join-Path $root 'tools\xextool\XexTool-windows-x64\XexTool.exe'
$xex = Join-Path $root 'Release\XCloud360.xex'
$embeddedFontScript = Join-Path $root 'embed_font.ps1'

if (!(Test-Path $msbuild)) { throw 'MSBuild 4.0 was not found.' }
if (!(Test-Path $project)) { throw 'Xbox 360 project file was not found.' }
if (!$SdkOnly -and !(Test-Path $xextool)) { throw 'Install XexTool in tools/xextool/XexTool-windows-x64, or use -SdkOnly for an SDK build without Retail inspection.' }
if ([string]::IsNullOrWhiteSpace($env:XEDK)) {
    throw 'XEDK is not configured for the Xbox 360 PowerPC compiler.'
}

Write-Host 'Step 2/3 - Building the native Xbox 360 PowerPC application'
& $msbuild $project /t:Build '/p:Configuration=Release;Platform=Xbox 360' /v:minimal
if ($LASTEXITCODE -ne 0) { throw 'PowerPC compilation failed.' }
if (!(Test-Path $xex)) { throw 'XEX was not found in Release.' }

if ($SdkOnly) {
    Write-Host ('SDK BUILD READY: ' + $xex)
    Write-Warning 'Retail format and console execution were not verified in -SdkOnly mode.'
    return
}

Write-Host 'Step 3/3 - Inspecting the XEX and converting it to Retail'
$info = (& $xextool -l $xex | Out-String)
if ($LASTEXITCODE -ne 0) { throw 'XexTool could not inspect the executable.' }
if ($info -notmatch '(?m)^\s+Retail\s*$' -or
    $info -notmatch 'All Media Types') {
    $retailDir = Join-Path $root 'obj\retail'
    New-Item -ItemType Directory -Force -Path $retailDir | Out-Null
    $temp = Join-Path $retailDir 'XCloud360-Retail.xex'
    & $xextool -m r -e e -r a -o $temp $xex
    if ($LASTEXITCODE -ne 0) { throw 'Devkit-to-Retail XEX conversion failed.' }
    $verified = (& $xextool -l $temp | Out-String)
    if ($LASTEXITCODE -ne 0 -or
        $verified -notmatch '(?m)^\s+Retail\s*$' -or
        $verified -notmatch 'All Media Types') {
        throw 'The converted XEX did not pass Retail verification.'
    }
    Copy-Item -LiteralPath $temp -Destination $xex -Force
}

$final = (& $xextool -l $xex | Out-String)
if ($LASTEXITCODE -ne 0 -or $final -notmatch '(?m)^\s+Retail\s*$') {
    throw 'The final XEX did not pass Retail verification.'
}
Write-Host ('READY: ' + $xex)
Write-Host ('SHA256: ' + (Get-FileHash -Algorithm SHA256 $xex).Hash)
Write-Host 'The smooth font is intended to be embedded in the XEX, with a bitmap fallback.'
Write-Host 'No Media/Fonts folder or ZIP is required.'
