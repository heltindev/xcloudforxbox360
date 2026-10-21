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
$renewalCompile = 'cl /nologo /EHsc /W4 /I"' + $projectRoot + '\src"'
foreach ($source in @('tests\oauth_renewal_tests.cpp','src\oauth_device.cpp','src\json_fields.cpp','src\xbox_device_flow.cpp','src\xbox_live_auth.cpp','src\secure_http.cpp','src\http_wire.cpp')) {
    $renewalCompile += ' "' + (Join-Path $projectRoot $source) + '"'
}
$renewalCompile += ' /Feoauth_renewal_tests.exe'
$sessionCompile = 'cl /nologo /EHsc /W4 /I"' + $projectRoot + '\src" /I"' + $projectRoot + '\third_party\cjson"'
foreach ($source in @('tests\session_tests.cpp','src\xcloud_session.cpp','src\secure_http.cpp','third_party\cjson\cJSON.c')) {
    $sessionCompile += ' "' + (Join-Path $projectRoot $source) + '"'
}
$sessionCompile += ' /Fesession_tests.exe'
$networkCompile = 'cl /nologo /EHsc /W4 /I"' + $projectRoot + '\src" "' + $projectRoot + '\tests\network_ready_tests.cpp" /Fenetwork_ready_tests.exe'
$cacheCompile = 'cl /nologo /EHsc /W4 /I"' + $projectRoot + '\src" "' + $projectRoot + '\tests\login_cache_tests.cpp" "' + $projectRoot + '\src\login_cache.cpp" "' + $projectRoot + '\src\login_cache_platform.cpp" "' + $projectRoot + '\src\secure_http.cpp" /Felogin_cache_tests.exe'
$lines = @('@echo off',('call "'+$VcVars+'" x86'),'if errorlevel 1 exit /b 1',
    ('cd /d "'+$build+'"'),$compile,'if errorlevel 1 exit /b 1','auth_http_tests.exe',
    'if errorlevel 1 exit /b 1',$renewalCompile,'if errorlevel 1 exit /b 1',
    'oauth_renewal_tests.exe','if errorlevel 1 exit /b 1',
    $sessionCompile,'if errorlevel 1 exit /b 1','session_tests.exe','if errorlevel 1 exit /b 1',
    $networkCompile,'if errorlevel 1 exit /b 1','network_ready_tests.exe','if errorlevel 1 exit /b 1',
    $cacheCompile,'if errorlevel 1 exit /b 1','login_cache_tests.exe','exit /b %errorlevel%')
$commandFile = Join-Path $build 'run.cmd'
[System.IO.File]::WriteAllLines($commandFile,$lines,[System.Text.Encoding]::Default)
& $env:ComSpec /d /c ('"'+$commandFile+'"')
if ($LASTEXITCODE -ne 0) { throw "Host tests failed: $LASTEXITCODE" }
