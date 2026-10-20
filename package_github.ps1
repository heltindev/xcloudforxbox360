$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$out = Join-Path $root 'XCloud360-GitHub-Source.zip'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

# Explicit allowlist: avoids SDK content, tool binaries, user secrets, build output,
# snapshots, .git data, executable binaries, caches and generated XPR fonts.
$files = @(
    'README.md',
    '.gitignore',
    'XCLOUDFORXBOX360.sln',
    'XCloud360.vcxproj',
    'build_release.ps1',
    'package_github.ps1'
)
$files += @(Get-ChildItem (Join-Path $root 'src') -File |
    Where-Object { $_.Extension -in @('.cpp', '.h') } |
    Sort-Object Name |
    ForEach-Object { 'src/' + $_.Name })
$files += @(Get-ChildItem (Join-Path $root 'docs') -File |
    Where-Object { $_.Extension -eq '.md' } |
    Sort-Object Name |
    ForEach-Object { 'docs/' + $_.Name })

foreach ($name in $files) {
    $file = Join-Path $root ($name.Replace('/', [IO.Path]::DirectorySeparatorChar))
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) {
        throw "Falta arquivo obrigatorio no pacote: $name"
    }
}
if (Test-Path -LiteralPath $out) { Remove-Item -LiteralPath $out -Force }
$archive = [System.IO.Compression.ZipFile]::Open($out, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($name in $files) {
        $file = Join-Path $root ($name.Replace('/', [IO.Path]::DirectorySeparatorChar))
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $file, $name,
            [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally {
    $archive.Dispose()
}
$check = [System.IO.Compression.ZipFile]::OpenRead($out)
try {
    $names = @($check.Entries | ForEach-Object { $_.FullName })
    if ($names.Count -ne $files.Count) { throw 'Arquivo ZIP incompleto.' }
    if (@($names | Where-Object { $_ -match '(?i)(\.xpr$|\.xex$|\.exe$|^tools/|^obj/|^Release/|^dist/|^bin/|^\.git/)' }).Count -gt 0) {
        throw 'O ZIP contem artefatos proibidos.'
    }
    foreach ($required in @('README.md','.gitignore','src/tcp_probe.cpp','src/udp_dns.cpp','docs/RGH_NETWORK.md')) {
        if ($names -notcontains $required) { throw "ZIP incompleto: $required" }
    }
} finally {
    $check.Dispose()
}
$size = [math]::Round((Get-Item $out).Length / 1MB, 2)
Write-Host ('GitHub: ' + $out)
Write-Host ('Arquivos-fonte: ' + $files.Count + ' / ' + $size + ' MB')
Write-Host 'Apenas codigo e documentacao, sem SDK, XEX, fonte XPR ou XexTool.'
Write-Host ('SHA256: ' + (Get-FileHash $out -Algorithm SHA256).Hash)
