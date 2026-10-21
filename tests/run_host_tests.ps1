param([string]$VcVars = 'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
if (!(Test-Path -LiteralPath $VcVars)) { throw "Compiler environment not found: $VcVars" }
$build = Join-Path $projectRoot 'obj\host-tests'
New-Item -ItemType Directory -Path $build -Force | Out-Null
foreach ($value in @($projectRoot,$build,$VcVars)) {
    if ($value -match '[%"!&|<>^\r\n]') { throw 'Unsupported shell character in build path.' }
}
$compile = 'cl /nologo /EHsc /W4 /I"' + $projectRoot + '\src" /I"' + $projectRoot + '\third_party\cjson"'
foreach ($source in @('tests\auth_http_tests.cpp','src\xbox_live_exchange.cpp','src\xbox_live_auth.cpp','src\secure_http.cpp','src\region_catalog.cpp','src\http_wire.cpp','third_party\cjson\cJSON.c')) {
    $compile += ' "' + (Join-Path $projectRoot $source) + '"'
}
$compile += ' /Feauth_http_tests.exe'
$lines = @('@echo off',('call "'+$VcVars+'" x86'),'if errorlevel 1 exit /b 1',
    ('cd /d "'+$build+'"'),$compile,'if errorlevel 1 exit /b 1','auth_http_tests.exe','exit /b %errorlevel%')
$commandFile = Join-Path $build 'run.cmd'
[System.IO.File]::WriteAllLines($commandFile,$lines,[System.Text.Encoding]::Default)
& $env:ComSpec /d /c ('"'+$commandFile+'"')
if ($LASTEXITCODE -ne 0) { throw "Host tests failed: $LASTEXITCODE" }
