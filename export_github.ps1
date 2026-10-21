# Export a reviewable source snapshot; never copy SDKs or account data.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$destination = Join-Path $root 'GITHUB-PAUSADO'
if (Test-Path -LiteralPath $destination) { throw 'GITHUB-PAUSADO already exists. Preserve/review it before choosing another export.' }
& (Join-Path $root 'REFERENCE_BASE\refresh_context.ps1') -Check
if ($LASTEXITCODE -ne 0) { throw 'Context check failed.' }
$files = @(& git -C $root ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0 -or !$files.Count) { throw 'Cannot enumerate public Git source candidates.' }
$compact = @('REFERENCE_BASE/AI_REFERENCE.md','REFERENCE_BASE/SOURCE_MAP.json','REFERENCE_BASE/refresh_context.ps1')
foreach ($relative in $files) {
    $name = $relative.Replace('\','/')
    if ([IO.Path]::IsPathRooted($relative) -or $name -match '(^|/)\.\.(/|$)' -or
        $name -match '(^|/)(\.git|GITHUB-PAUSADO|auth_config\.h|obj|bin|Release|Debug|assets/Fonts|tools/xextool)(/|$)' -or
        $name -match '\.authcache' -or $name -match '\.(xex|exe|xpr|obj|pdb|lib|suo|user|log|zip|rar|pkg|elf|sprx)$') {
        throw "Non-public source candidate: $name"
    }
    if ($name.StartsWith('REFERENCE_BASE/') -and $compact -notcontains $name) { throw "Raw reference candidate: $name" }
    if ((Get-Item -LiteralPath (Join-Path $root $relative)).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Linked source is not exported: $name" }
}
New-Item -ItemType Directory -Path $destination | Out-Null
foreach ($relative in $files) {
    $target = Join-Path $destination $relative
    New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $relative) -Destination $target
}
[xml]$project = Get-Content -LiteralPath (Join-Path $destination 'XCloud360.vcxproj') -Raw
$ns = New-Object Xml.XmlNamespaceManager($project.NameTable)
$ns.AddNamespace('p','http://schemas.microsoft.com/developer/msbuild/2003')
foreach ($item in $project.SelectNodes('//p:ClCompile[@Include]|//p:ClInclude[@Include]',$ns)) {
    $include = $item.GetAttribute('Include')
    if ($include -eq 'obj\generated\embedded_font_xpr.cpp') { continue }
    if (!(Test-Path -LiteralPath (Join-Path $destination $include) -PathType Leaf)) { throw "Project source missing: $include" }
}
$manifest = foreach ($relative in $files) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $destination $relative) -Algorithm SHA256).Hash
    $hash + '  ' + $relative.Replace('\','/')
}
[IO.File]::WriteAllLines((Join-Path $destination 'GITHUB_MANIFEST.sha256'),$manifest,[Text.Encoding]::UTF8)
Write-Output ('Source files exported: '+$files.Count)
Write-Output ('Ready for manual GitHub upload: '+$destination)
Write-Output 'Project source references checked. No SDK, credentials, build binaries or Git history copied.'
