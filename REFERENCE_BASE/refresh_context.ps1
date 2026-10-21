param([switch]$Check)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$utf8 = New-Object System.Text.UTF8Encoding($false)
$specs = @(
 'src/main.cpp|active|Entry point and ABI|main',
 'src/app.cpp|active|UI and flow|poll_account',
 'src/platform_xbox360.cpp|active|D3D9 and XInput|initialize',
 'src/xbox_account.cpp|active|Orchestration and test session|perform_session',
 'src/xbox_device_flow.cpp|active|OAuth refresh and transfer|request_transfer_token',
 'src/login_cache.h|active|Protected cache contract|Store',
 'src/login_cache.cpp|active|Cache format and binding|Store::load;Store::save',
 'src/login_cache_platform.cpp|active|Native protection and atomic writes|protect;write_atomic',
 'tests/login_cache_tests.cpp|active|Host DPAPI protection and cache failures|main',
 'src/xbox_live_exchange.cpp|active|Xbox User XSTS and login|exchange',
 'src/xcloud_session.cpp|active|Creation queue connection cleanup|Session::start;Session::poll;Session::stop',
 'src/xboxtls_backend.cpp|active|HTTPS and allowlists|cloud_request',
 'src/http_wire.cpp|active|Framing HTTP|response_complete',
 'src/tcp_probe.cpp|active|TCP diagnostics|attempt_tcp',
 'src/network_ready.h|active|Bounded IP acquisition policy|classify;wait',
 'src/xbox_network_ready.h|active|SYSAPP address lookup|sample',
 'tests/network_ready_tests.cpp|active|Simulated delayed DHCP and rollover|main',
 'third_party/xboxtls/src/platform/xbox360/platform_xbox360.cpp|active|Multi-IP TCP connect and readiness|connect_tcp',
 'third_party/xboxtls/include/xboxtls/xbox360_platform.h|active|TLS network diagnostics|XboxNetworkDiagnostic',
 'src/xcloud_bridge.cpp|active|Channel and RTP adapter|receive_video',
 'src/xcloud_input.cpp|active|Controller encoding|gamepad',
 'src/xcloud_control.cpp|active|JSON messages|handshake',
 'src/h264_rtp_receiver.cpp|active|H264 reassembly|Receiver::push',
 'src/rtp_packet.cpp|active|RTP parsing and bytes|parse',
 'src/webrtc_xbox.h|active|Prototype without cpp implementation|create_session',
 'tests/auth_http_tests.cpp|active|Authentication and HTTP regressions|main',
 'tests/oauth_renewal_tests.cpp|active|Simulated refresh and transfer|main',
 'tests/session_tests.cpp|active|Simulated session|lifecycle',
 'tests/run_host_tests.ps1|active|Test command|cl',
 'XCloud360.vcxproj|active|Compiled sources and libraries|AdditionalDependencies',
 'build_release.ps1|active|PowerPC build and Retail inspection|Step 2',
 'GR33N-main/source/auth.c|reference|Authentication sequence|gssv',
 'GR33N-main/source/session.c|reference|Signaling and states|step_play;step_connect;step_sdp_offer;step_ice_post',
 'GR33N-main/source/webrtc.c|reference|libpeer adaptation|peer_connection',
 'GR33N-main/source/vjitter.c|reference|Reordering and packet loss|NACK',
 'GR33N-main/source/decoder.c|reference|PS3-specific decoder|cellVdec',
 'GR33N-main/source/aud.c|reference|Opus and PS3 audio|opus',
 'GR33N-main/source/xcmsg.c|reference|Application messages|Handshake',
 'GR33N-main/deps/build-mbedtls.sh|reference|PS3 DTLS build|PS3DEV',
 'GR33N-main/deps/build-libsrtp.sh|reference|PS3 SRTP build|PS3DEV',
 'GR33N-main/deps/build-usrsctp.sh|reference|PS3 SCTP build|PS3DEV',
 'GR33N-main/deps/build-libpeer.sh|reference|Peer dependencies|PS3DEV',
 'GR33N-main/deps/build-opus.sh|reference|PS3 Opus build|PS3DEV',
 'Harissa64-master/platform/xbox360/xb_net.cpp|reference|Xbox 360 network|XNet',
 'Harissa64-master/platform/xbox360/xb_audio.cpp|reference|XAudio2 output|XAudio2',
 'Harissa64-master/platform/xbox360/xb_art.cpp|reference|Textures|Texture',
 'xbox_launcher-main/pubspec.yaml|reference|Flutter Dart WebView|webview_windows'
)
$entries = @()
foreach ($spec in $specs) {
    $fields = $spec.Split('|')
    $relative = $fields[0]
    $candidates = @()
    if ($fields[1] -eq 'active') { $candidates += Join-Path $projectRoot $relative }
    else {
        $candidates += Join-Path $PSScriptRoot $relative
        $candidates += Join-Path (Join-Path $PSScriptRoot 'ORIGINALS') $relative
    }
    $found = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
    $entry = [ordered]@{kind=$fields[1];topic=$fields[2];focus=$fields[3]}
    if ($found) {
        $entry.path = $found.Substring($projectRoot.Length+1).Replace('\','/')
        $entry.bytes = (Get-Item -LiteralPath $found).Length
        $entry.sha256 = (Get-FileHash -LiteralPath $found -Algorithm SHA256).Hash
    } else { $entry.path=$relative; $entry.missing=$true }
    $entries += [PSCustomObject]$entry
}
$active = @($entries | Where-Object { $_.kind -eq 'active' })
if (@($active | Where-Object { $_.missing }).Count) { throw 'An active source required by the compact index is missing.' }
$canonical = ($active | ForEach-Object { $_.path+'|'+$_.sha256 }) -join ([char]10)
$sha = [Security.Cryptography.SHA256]::Create()
try { $digest = ([BitConverter]::ToString($sha.ComputeHash($utf8.GetBytes($canonical)))).Replace('-','') }
finally { $sha.Dispose() }
$map = [ordered]@{format=1;root='.';read_first='AI_CONTEXT.md';active_sources_sha256=$digest;files=$entries}
$json = ($map | ConvertTo-Json -Depth 6) + ([char]10)
$destination = Join-Path $PSScriptRoot 'SOURCE_MAP.json'
if ($Check) {
    if (!(Test-Path -LiteralPath $destination)) { throw 'Compact index is missing.' }
    $stored = [IO.File]::ReadAllText($destination) | ConvertFrom-Json
    if ($stored.active_sources_sha256 -ne $digest -or $stored.files.Count -ne $entries.Count) {
        throw 'Compact active-source index is stale. Run refresh_context.ps1 without -Check.'
    }
    foreach ($entry in $entries) {
        if ($entry.kind -eq 'reference' -and $entry.missing) { continue }
        $previous = @($stored.files | Where-Object { $_.kind -eq $entry.kind -and $_.topic -eq $entry.topic })
        if ($previous.Count -ne 1 -or $previous[0].sha256 -ne $entry.sha256 -or
            $previous[0].path -ne $entry.path -or $previous[0].focus -ne $entry.focus) {
            throw ('Compact entry is stale: '+$entry.path)
        }
    }
    Write-Output 'Compact index is current.'
} else {
    [IO.File]::WriteAllText($destination,$json,$utf8)
    Write-Output ('Indexed selected files: '+$entries.Count)
    Write-Output ('Active sources SHA256: '+$digest)
}
