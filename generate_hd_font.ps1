# Create a local high-density Segoe UI font resource using the installed Xbox 360 SDK.
# The generated font files and font-maker executable must never be published.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$fontDir = Join-Path $root 'assets\Fonts'
$fontXpr = Join-Path $fontDir 'SegoeUI_42.xpr'
$generatorDir = Join-Path $root 'obj\FontMakerCLI'
$fontGenerator = Join-Path $generatorDir 'FontMakerCLI.exe'
if (Test-Path -LiteralPath $fontXpr) {
    Write-Host 'High-density 42pt atlas already exists locally.'
    return
}
if ([string]::IsNullOrWhiteSpace($env:XEDK)) {
    throw 'XEDK is required for local font generation.'
}
$xdkFonts = Join-Path $env:XEDK 'Source\Tools\Graphics\FontMaker'
$bundler = Join-Path $env:XEDK 'bin\win32\bundler.exe'
$vc = 'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat'
foreach ($needed in @($vc,$bundler,
    (Join-Path $xdkFonts 'FontMaker_CmdLine.cpp'),
    (Join-Path $xdkFonts 'Glyphs.cpp'))) {
    if (!(Test-Path -LiteralPath $needed)) {
        throw "Missing locally installed SDK/compiler file: $needed"
    }
}
New-Item -ItemType Directory -Force -Path $fontDir,$generatorDir | Out-Null
if (!(Test-Path -LiteralPath $fontGenerator)) {
    $cliBuild = Join-Path $generatorDir 'build_fontmaker.cmd'
    $command = @"
@echo off
call "$vc" x86
if errorlevel 1 exit /b 2
cd /d "$generatorDir"
cl /nologo /O2 /EHsc /MT /Y- /D_CRT_SECURE_NO_WARNINGS /I"$xdkFonts" /Fe"$fontGenerator" "$xdkFonts\FontMaker_CmdLine.cpp" "$xdkFonts\Glyphs.cpp" /link /SUBSYSTEM:CONSOLE /INCREMENTAL:NO gdi32.lib user32.lib
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText($cliBuild,$command,[Text.Encoding]::ASCII)
    & cmd.exe /d /c ('"' + $cliBuild + '"')
    if ($LASTEXITCODE -ne 0 -or !(Test-Path $fontGenerator)) {
        throw 'Could not compile the XDK FontMaker command-line utility.'
    }
}
Push-Location $fontDir
try {
    & $fontGenerator -f 'Segoe UI' -fs 42 -tw 1024 -th 512 -gr 32 126 -out 'SegoeUI_42'
    if ($LASTEXITCODE -ne 0) {
        throw 'FontMaker could not rasterize the Segoe UI 42pt glyphs.'
    }
    if (!(Test-Path 'SegoeUI_42.abc') -or !(Test-Path 'SegoeUI_42.tga')) {
        throw 'The font generator did not produce both artifacts.'
    }
    $rdf = @'
<?xml version="1.0" encoding="utf-8" ?>
<RDF Version="XPR2">
<Texture Name="FontTexture" Source="SegoeUI_42.tga" Format="D3DFMT_LIN_A4R4G4B4" Levels="1" />
<UserData Name="FontData" DataFile="SegoeUI_42.abc" />
</RDF>
'@
    [IO.File]::WriteAllText((Join-Path $fontDir 'SegoeUI_42.rdf'),$rdf,[Text.Encoding]::ASCII)
    & $bundler (Join-Path $fontDir 'SegoeUI_42.rdf') -o $fontXpr
    if ($LASTEXITCODE -ne 0 -or !(Test-Path $fontXpr)) {
        throw 'Unable to bundle the high-density font resource.'
    }
    $length = (Get-Item -LiteralPath $fontXpr).Length
    if ($length -lt 1000 -or $length -gt 2MB) {
        throw 'The bundled font has an unexpected size.'
    }
    Write-Host "Local Segoe UI 42pt atlas ready ($length bytes)."
} finally {
    Pop-Location
}
