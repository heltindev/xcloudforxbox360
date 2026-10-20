# Native Xbox 360 PowerPC architecture

**Primary platform:** real Xbox 360 with RGH/RGH 3. The Xenon CPU uses three PowerPC cores (six hardware threads) and a big-endian execution environment. The project builds a native Xbox 360 Retail-format `.xex`, not a Windows executable or web browser.

## Target pipeline (not yet implemented)

```text
Xbox 360 XInput -> controller mapping ---------> streaming input channel
                           |
                           v
XCloud360 application -> DNS/TCP/UDP -> TLS/DTLS/SRTP/WebRTC -> cloud session
                           |
                  received media packets
                           |
             H.264/negotiated codec decoder -> Xenos GPU -> TV
                       audio decoder -> Xbox audio output
```

The project currently contains **UI and network diagnostics**, plus preliminary HTTP/JSON/OAuth/RTP/H.264 parsers. Those modules do **not** provide authenticated streaming, negotiated media channels, codec decoding or functioning Microsoft sign-in.

## Two Xbox 360 execution environments

| Execution path | Format | Startup method | Status |
| --- | --- | --- | --- |
| Xbox 360 XDK/XNet | `XCloud360.xex` | Aurora or XeXMenu on RGH | Main project; external TCP times out |
| libxenon/XeLL | PowerPC ELF (`xenon.elf`) | XeLL Reloaded | Separate boot-only source experiment; not compiled |

A XeLL ELF cannot simply be copied to Aurora as an XEX. A new network and rendering backend would be needed. Neither environment automatically provides TLS/DTLS/WebRTC.

## Priorities

1. Fix or replace the external network transport and prove TCP/UDP on the actual Xbox 360.
2. Port a secure, verified TLS stack with certificate validation.
3. Establish **legitimately authorized** Microsoft sign-in and permission to use relevant xCloud APIs.
4. Negotiate a cloud session and media transport appropriate to the actual current service.
5. Validate video decoding, audio, controller packets and memory limits (512 MB shared RAM).
6. Demonstrate stable interactive Fortnite on real hardware before declaring the project functional.

The latest RGH test returned **DNS: 1 IP**, but outbound TCP to Xbox.com and 1.1.1.1 port 443 timed out with `10060`, source `8`. No root cause has been confirmed.

See [RGH_NETWORK.md](RGH_NETWORK.md), [REFERENCES.md](REFERENCES.md) and [the full roadmap](../ROADMAP.md).
