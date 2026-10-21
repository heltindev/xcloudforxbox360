# Update the GitHub-ready SOURCE FOLDER only. This script intentionally
# does NOT compile, create a ZIP, publish to GitHub or modify Release/.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$mirror = Join-Path $root 'XCloud360-GitHub-Source'
$research = Join-Path $root 'PASTACODIGOBASE'

if (!(Test-Path -LiteralPath (Join-Path $root 'XCloud360.vcxproj'))) {
    throw 'Run package_github.ps1 from the main XCloud360 project folder.'
}
if (!(Test-Path -LiteralPath $research -PathType Container)) {
    throw 'Missing private PASTACODIGOBASE. Inspect repository layout.'
}
New-Item -ItemType Directory -Path $mirror -Force | Out-Null

# Only first-party Xbox 360 sources, portable third-party library source,
# license notices and docs may be included. Do NOT copy any PS3/PS4
# reference checkout, toolchain executable, cache, secret or XEX.
$files = New-Object System.Collections.Generic.List[string]
foreach ($name in @(
    '.gitignore', 'README.md', 'ROADMAP.md', 'STATUS_DO_PROJETO.md',
    'XCLOUDFORXBOX360.sln', 'XCloud360.vcxproj',
    'auth_config.example.h', 'xex.xml',
    'build_release.ps1', 'embed_font.ps1', 'package_github.ps1'
)) {
    $files.Add($name)
}
foreach ($folder in @('src', 'docs', 'third_party')) {
    $path = Join-Path $root $folder
    if (!(Test-Path -LiteralPath $path)) {
        throw "Required source folder missing: $folder"
    }
    Get-ChildItem -LiteralPath $path -Recurse -File | Where-Object {
        $_.Extension -in @('.cpp', '.c', '.h', '.md', '.ps1', '.xml', '.txt') -or
        $_.Name -in @('LICENSE', 'Makefile')
    } | ForEach-Object {
        $relative = $_.FullName.Substring($root.Length + 1)
        $files.Add($relative)
    }
}

$files = @($files | Sort-Object -Unique)
foreach ($relative in $files) {
    $src = Join-Path $root $relative
    $dst = Join-Path $mirror $relative
    if (!(Test-Path -LiteralPath $src -PathType Leaf)) {
        throw "Required source file is missing: $relative"
    }
    New-Item -ItemType Directory -Path (Split-Path $dst -Parent) -Force |
        Out-Null
    Copy-Item -LiteralPath $src -Destination $dst -Force
    $srcHash = (Get-FileHash -LiteralPath $src -Algorithm SHA256).Hash
    $dstHash = (Get-FileHash -LiteralPath $dst -Algorithm SHA256).Hash
    if ($srcHash -ne $dstHash) {
        throw "Hash mismatch in GitHub source mirror: $relative"
    }
}

# The PS3-only inspector was exported by a previous version of the script.
# Its new location is PRIVATE under PASTACODIGOBASE.
$obsoleteResearch = Join-Path $mirror 'research_tools'
if (Test-Path -LiteralPath $obsoleteResearch) {
    $known = @('inspect_pkg.ps1')
    $extras = @(Get-ChildItem -LiteralPath $obsoleteResearch -Recurse -File |
        Where-Object { $known -notcontains $_.Name })
    if ($extras.Count -gt 0) {
        throw 'Review unknown files inside old GitHub research_tools before deleting.'
    }
    Remove-Item -LiteralPath $obsoleteResearch -Recurse -Force
}

# This separate libxenon/XeLL experiment was archived under the private
# PASTACODIGOBASE/ tree; it is not a native .xex build dependency.
$oldExperiment = Join-Path $mirror 'experimental_xell'
if (Test-Path -LiteralPath $oldExperiment) {
    $archived = Join-Path $research 'XBOX360_EXPERIMENTOS\experimental_xell'
    if (!(Test-Path -LiteralPath $archived)) {
        throw 'Archived XeLL source missing; refusing to delete the old mirror.'
    }
    $oldFiles = @(Get-ChildItem -LiteralPath $oldExperiment -Recurse -File)
    foreach ($file in $oldFiles) {
        $relative = $file.FullName.Substring($oldExperiment.Length).TrimStart('\')
        $archivedFile = Join-Path $archived $relative
        if (!(Test-Path -LiteralPath $archivedFile) -or
            (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -ne
            (Get-FileHash -LiteralPath $archivedFile -Algorithm SHA256).Hash) {
            throw "Unknown or changed XeLL mirror file; refusing deletion: $relative"
        }
    }
    Remove-Item -LiteralPath $oldExperiment -Recurse -Force
}

$forbidden = @(Get-ChildItem -LiteralPath $mirror -Recurse -File -Force |
    Where-Object {
        $_.Name -eq 'auth_config.h' -or
        $_.Extension -in @(
            '.xex', '.exe', '.xpr', '.pkg', '.elf', '.zip', '.sdf',
            '.suo', '.pdb', '.obj', '.exp', '.lib', '.ttf', '.at3',
            '.wav', '.ogg', '.png', '.jpg', '.bin'
        )
    })
if ($forbidden.Count -gt 0) {
    throw "Private or binary file detected in the mirror: $($forbidden[0].FullName)"
}
foreach ($required in @(
    'src\xbox_account.cpp', 'src\xboxtls_backend.cpp',
    'src\tls_trust.cpp', 'auth_config.example.h',
    'third_party\xboxtls\LICENSE',
    'third_party\xboxtls\src\tls\connection.cpp',
    'docs\TLS_ACCOUNT_INTEGRATION.md',
    'docs\UPDATE_SOURCE_ORGANIZATION.md'
)) {
    if (!(Test-Path -LiteralPath (Join-Path $mirror $required))) {
        throw "Incomplete mirror: $required"
    }
}
if (Test-Path -LiteralPath (Join-Path $mirror 'PASTACODIGOBASE')) {
    throw 'Private PS3/PS4 reference source leaked into the GitHub mirror.'
}

Write-Host "GitHub-ready source mirror: $mirror"
Write-Host "Verified files: $($files.Count)"
Write-Host 'No ZIP, no compilation, no GitHub push, no Release changes.'
Write-Host 'PASTACODIGOBASE and local auth_config.h remain private.'
