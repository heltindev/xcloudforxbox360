$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$font = Join-Path $root 'assets\Fonts\SegoeUI_24.xpr'
$generated = Join-Path $root 'obj\generated\embedded_font_xpr.cpp'

# This step runs locally before the PowerPC compiler. The Microsoft SDK
# font bytes are never checked into Git or shipped as a separate file.
if (!(Test-Path -LiteralPath $font)) {
    if ([string]::IsNullOrWhiteSpace($env:XEDK)) {
        throw 'XEDK not configured. Install/configure an authorized Xbox 360 SDK.'
    }
    $bundler = Join-Path $env:XEDK 'bin\win32\bundler.exe'
    $rdf = Join-Path $env:XEDK 'Source\Samples\Media\Fonts\SegoeUI_24.rdf'
    if (!(Test-Path $bundler) -or !(Test-Path $rdf)) {
        throw 'Bundler / SegoeUI_24.rdf was not found in the local Xbox 360 SDK.'
    }
    New-Item -ItemType Directory -Force -Path (Split-Path $font) | Out-Null
    & $bundler $rdf -o $font
    if ($LASTEXITCODE -ne 0 -or !(Test-Path $font)) { throw 'Font Bundler failed.' }
}
if ((Test-Path -LiteralPath $generated) -and
    (Get-Item -LiteralPath $generated).LastWriteTimeUtc -ge
    (Get-Item -LiteralPath $font).LastWriteTimeUtc) {
    Write-Host 'Fonte interna ja gerada: nenhum arquivo XPR externo sera necessario.'
    return
}

$bytes = [System.IO.File]::ReadAllBytes($font)
if ($bytes.Length -lt 128 -or $bytes.Length -gt 2MB) {
    throw 'Unexpected local XPR font length; refusing to embed.'
}
New-Item -ItemType Directory -Force -Path (Split-Path $generated) | Out-Null
$builder = New-Object System.Text.StringBuilder
[void]$builder.AppendLine('// Generated from a locally installed Xbox 360 SDK. DO NOT PUBLISH.')
[void]$builder.AppendLine('#include "embedded_font_xpr.h"')
[void]$builder.AppendLine('__declspec(align(16)) extern const unsigned char g_embedded_font_xpr[] = {')
for ($i=0; $i -lt $bytes.Length; $i+=16) {
    $end = [Math]::Min($i+15,$bytes.Length-1)
    $line = New-Object System.Collections.Generic.List[string]
    for ($j=$i; $j -le $end; ++$j) {
        $line.Add('0x' + $bytes[$j].ToString('X2'))
    }
    [void]$builder.Append('  ')
    [void]$builder.Append(($line -join ', '))
    [void]$builder.AppendLine(',')
}
[void]$builder.AppendLine('};')
[void]$builder.AppendLine(('extern const unsigned int g_embedded_font_xpr_size = {0}u;' -f $bytes.Length))
[System.IO.File]::WriteAllText($generated,$builder.ToString(),
                               [System.Text.Encoding]::ASCII)
Write-Host ('Fonte XPR incorporada na unidade de compilacao local: ' + $generated)
Write-Host 'Nao publicar o .cpp gerado: contem recurso da SDK Microsoft.'
