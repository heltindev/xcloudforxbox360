# Optional Xbox 360 network research using XeLL / libxenon

**Main XCloud360 application:** Xbox 360 Retail `.xex` launched by Aurora/XeXMenu.  
**Separate experiment:** a PowerPC ELF started from **XeLL Reloaded**, not from the XEX loader.

Libxenon is a **bare-metal Xbox 360 homebrew development environment**, not a general Linux installation on the console. XeLL can load a compatible PowerPC ELF. The developer's PC may use Linux to build the toolchain, but the runtime target remains **the real Xbox 360**, without an intermediary PC.

## What exists now

A deliberately isolated folder `experimental_xell/` was added with a minimal `Makefile` and `source/main.c`, derived structurally from the documented Free60 `xenon-examples` layout. It initializes a console screen and polls the controller for Y/Guide to exit.

**This example has not been compiled; it has no networking and is not a streaming client.** It is meant to validate the alternative startup path only.

## Planned investigation

1. Set up an authorized, compatible libxenon cross-compiler and `DEVKITXENON` environment variable on the developer's build machine.
2. From `experimental_xell/`, use `make` to compile a native PowerPC ELF32.
3. Copy the resulting ELF32 to a FAT32 USB drive as `xenon.elf` and load it via **an already-installed XeLL**.
4. Confirm GPU initialization, displayed text, controller polling and clean exit.
5. Inspect the actual network APIs and lwIP configuration in the installed libxenon version.
6. Implement a minimal DHCP/DNS/TCP/UDP hardware test and compare it with the XDK/XNet timeouts.
7. Only if a better backend is proved viable, plan shared transport interfaces and a port of the application UI.

**Do not flash/update NAND or change XeLL merely for these tests.** Libxenon does not automatically implement modern TLS, Microsoft OAuth, WebRTC, video decoding or xCloud.

Upstream references:
- [Free60 xenon-examples](https://github.com/Free60Project/xenon-examples)
- [Free60 libxenon](https://github.com/Free60Project/libxenon)
- [XeLL Reloaded](https://github.com/Free60Project/xell-reloaded)
