# Validates public source candidates without exporting another source tree.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
foreach ($required in @('AI_CONTEXT.md','XCloud360.vcxproj','src\main.cpp','src\xcloud_session.cpp','docs\XEX_GAPS.md','docs\VALIDATION_CURRENT.md','REFERENCE_BASE\AI_REFERENCE.md')) {
    if (!(Test-Path -LiteralPath (Join-Path $root $required) -PathType Leaf)) { throw "Missing project file: $required" }
}
& (Join-Path $root 'REFERENCE_BASE\refresh_context.ps1') -Check
$files = @(& git -C $root ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0) { throw 'Unable to enumerate the root Git repository.' }
$compact = @('REFERENCE_BASE/AI_REFERENCE.md','REFERENCE_BASE/SOURCE_MAP.json','REFERENCE_BASE/refresh_context.ps1')
foreach ($file in $files) {
    $normalized = $file.Replace('\','/')
    if ($normalized -match '(^|/)(auth_config\.h|obj|bin|Release|assets/Fonts|tools/xextool)(/|$)' -or
        $normalized -match '\.authcache' -or
        $normalized -match '\.(xex|exe|xpr|obj|pdb|rar|zip|pkg|sprx|elf|lib|sdf|suo)$') {
        throw "Private/build file is a Git candidate: $normalized"
    }
    if ($normalized.StartsWith('REFERENCE_BASE/') -and $compact -notcontains $normalized) {
        throw "Original reference is a Git candidate: $normalized"
    }
}
Write-Output ('Public source candidates checked: '+$files.Count)
Write-Output ('Canonical project: '+$root)
Write-Output 'No duplicate source export. No commit, tag or GitHub publication.'
