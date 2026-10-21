> Development paused by the author on 2026-10-21. Forks are welcome. xCloud is **not playable** yet. See README.md and docs/VALIDATION_HANDOFF.md. The following is the previous development roadmap.

# Roadmap and checklist — 2026-10-21

Goal: Xbox Cloud Gaming directly on an RGH/JTAG Xbox 360, without a PC intermediary during gameplay.

## Completed in this codebase

- [x] Protected refresh-token cache, code-free resumption, rotation, and local deletion.
- [ ] Validate native cache protection and resumption with a real account on console.
- [x] Native green interface with minimal text and controller navigation.
- [x] Microsoft Device Code and Xbox User/XSTS/xCloud login exchange code.
- [x] Nested JSON parsing and returned-region validation.
- [x] gsToken retained only in RAM, with expiration and clearing.
- [x] Cancellation between authentication stages.
- [x] NET-10060-A1 diagnostics: bounded IP wait, multi-IP HTTPS, and network screen.
- [x] Simulated DHCP/timeout/rollover tests and Visual Studio 2010 build.
- [x] Experimental session creation, queue, connection, renewal, and termination code.
- [x] HTTPS response reading ends after complete Content-Length or chunked bodies.
- [x] Simulated authentication, credential, and HTTP tests passed.
- [x] PowerPC Release compilation and Retail XEX verification passed.

Code and build checkmarks do not imply real account or physical-console validation.

## Requested checklist

| Item | Current status |
| --- | --- |
| ☐ Validate real authentication | Microsoft consent, Xbox User, XSTS, and xCloud login not validated in this revision |
| ☐ Validate Xbox 360 networking | TCP 10060 remains unresolved on hardware; HTTP read fixes do not prove a fix to the TCP connection |
| ◐ Implement gameplay session | Creation, queue, connection, and termination code exist; real validation and continuous-media transition remain pending |
| ◐ Manage session credentials | RAM retention, expiry, clearing, test-session use, and attempted renewal implemented; real validation and continuous recovery pending |
| ☐ Integrate WebRTC | ICE/DTLS/SRTP/SCTP negotiation and transport not integrated |
| ☐ Decode video | No integrated H.264 decoder or rendering |
| ☐ Play audio | No integrated Opus playback or video synchronization |
| ☐ Send controller input | Local formatters exist, but no real remote channel is connected |
| ☐ Test and optimize on console | Memory, performance, latency, reconnection, and stability unvalidated |

## Next steps

- [ ] Validate real networking and authentication on Xbox 360.
- [x] Connect credentials to the test session and implement renewal attempts.
- [ ] Validate renewal against the real service.
- [x] Implement session creation, queue, connect, and termination code.
- [ ] Validate a real session and keep it alive during media.
- [ ] Integrate WebRTC.
- [ ] Integrate video, audio, and controls.
- [ ] Test and optimize on physical hardware.

Only declare streaming functional after a real session with video, audio, and controls on Xbox 360. This revision cannot play games.

See [pre-release notes](RELEASE_NOTES.md) and [project status](PROJECT_STATUS.md).
