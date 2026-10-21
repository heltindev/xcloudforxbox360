# Useful AI reference base

This is a semantic summary, not a source-code ZIP. Reading binary archives does not reduce the context needed to understand a project. Start with `../AI_CONTEXT.md`, and open original source only to clarify a specific function.

## Useful references

| Reference | Useful for | Do not copy directly |
| --- | --- | --- |
| GR33N | Authentication/session sequence, messages, separation of WebRTC, video, and audio | PSL1GHT APIs, cellVdec, PS3 audio, and PS3-compiled libraries |
| Harissa64 | D3D9/XInput/XAudio2 organization and Xbox 360 networking | Code, art, or fonts without observing licenses; emulator behavior does not prove streaming |
| xbox_launcher | Visual inspiration and navigation | Flutter/Dart with a Windows WebView according to pubspec.yaml; not a native XDK/D3D9 implementation |
| XeLL/libxenon | Alternative boot/network research | Not part of the XDK `.xex` executable |
| PKGs, extractions, older builds | Private history, excluded from default reading | Binaries, images, caches, and packages do not implement the native peer |

Local reference copies provide evidence of implementation, not a guarantee they still work. Summaries do not modify original licenses. Do not automatically copy GPL code into this project.

## Targeted reading

Paths are relative to this folder. The index generator looks in the original location and `ORIGINALS/`.

- `GR33N-main/source/auth.c`: token sequence and separation of service and transfer tokens.
- `GR33N-main/source/session.c`: session creation, states, connection, SDP/ICE, termination.
- `GR33N-main/source/webrtc.c`: libpeer integration and callbacks; not a standalone WebRTC library.
- `GR33N-main/source/vjitter.c`: video packet ordering, losses, and recovery.
- `GR33N-main/source/decoder.c`: PS3-specific cellVdec adaptation with no confirmed Xbox equivalent.
- `GR33N-main/source/aud.c`: Opus, queues, and PS3 audio output.
- `GR33N-main/source/xcmsg.c`: application and controller messages.
- `GR33N-main/deps/build-*.sh`: mbedTLS, libSRTP, usrsctp, libpeer, and Opus dependencies. These scripts build for PS3, not XDK.
- `Harissa64-master/platform/xbox360/xb_net.cpp`: sockets/network.
- `Harissa64-master/platform/xbox360/xb_audio.cpp`: XAudio2.
- `Harissa64-master/platform/xbox360/xb_art.cpp`: textures.
- `xbox_launcher-main/pubspec.yaml`: confirms Flutter/Dart/WebView.

## Active project flow

Current:
`main -> App -> XboxAccount -> OAuth -> Xbox User -> XSTS -> xCloud login -> Session test -> termination`.

Future:
`Provisioned Session -> SDP/ICE signaling -> WebRTC peer -> authenticated/decrypted SRTP -> RTP reordering -> Bridge/H264 Receiver -> decoders -> D3D9/XAudio2`.

Controls:
`XInput -> xcloud_input -> SCTP/DTLS/ICE DataChannel`.

The Bridge accepts already authenticated/decrypted RTP. Never feed raw internet datagrams into it. Do not declare a session secure merely because HTTP 200 was returned or an SDP offer was generated.

## Design decisions that avoid rework

- The `XCLOUDFORXBOX360` root contains the complete GitHub project; do not maintain two active `src` trees.
- `third_party/xboxtls` and `third_party/cjson` are active dependencies; do not archive them with reference material.
- Preserve certificate and hostname validation; do not disable TLS to work around a timeout.
- A D3D9 texture is not an H.264 decoder.
- Do not confuse TLS/TCP with DTLS/UDP or assume DTLS 1.3 support.
- Use bounded queues and avoid per-packet allocations when integrating streaming.
- The current test releases the remote machine after provisioning. Media integration must change this lifecycle while preserving cleanup/cancellation.

## Sign-in cache

Refresh tokens now use protected persistence. Do not confuse them with gsToken/XSTS held in memory. Read `../docs/LOGIN_CACHE.md` when modifying authentication, and never include `*.authcache*` files in prompts or publications.

## Refresh without wasting context

Run `refresh_context.ps1` after source changes. It updates only `SOURCE_MAP.json`: relative paths, reading focus, byte count, and SHA-256. It neither extracts secrets nor concatenates the whole tree. `-Check` detects a stale map. Review the semantic summary whenever behavior changes.

Original references were preserved. Material outside this summary is excluded from initial reading and publication. Some folders may remain at original paths while in use.

In a public clone, the originals might not be present. `-Check` validates active sources and only those references that are available locally. The map does not replace targeted source inspection or prove the references work.

## Paused handoff — 2026-10-21

The author paused development and allows continuation of their contribution via forks. `README.md` contains the current checklist and `docs/BUILD_VS2010.md` covers local setup/build. `prepare_vs2010.ps1` prepares the example `auth_config.h`, font, and ATG. The `GITHUB-PAUSADO` snapshot is only for publication; raw research, credentials, SDK files, and binaries are not included.
