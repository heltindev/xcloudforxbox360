#include "app.h"
#include "platform.h"

// Xbox 360 Xenon is PowerPC. This executable is NOT a Windows client.
// The XDK project must compile for the real Xbox 360 target.
#ifndef _XBOX
#error XCloud360 requires the Xbox 360 XDK target (PowerPC).
#endif

#if !defined(_M_PPC) && !defined(_PPC_) && !defined(__powerpc__)
#error XCloud360 requires a PowerPC compiler, not x86 or x64.
#endif

int main() {
    // Xbox 360 PowerPC is big-endian and uses 32-bit pointers.
    // Detect an incompatible build before starting the application.
    const unsigned int byte_order = 0x01020304u;
    const unsigned char* bytes =
        reinterpret_cast<const unsigned char*>(&byte_order);
    // Compile-time ABI requirement: native 32-bit Xbox 360 pointers.
    typedef char XCloud360Requires32BitPointers[(sizeof(void*) == 4) ? 1 : -1];
    (void)sizeof(XCloud360Requires32BitPointers);
    if (bytes[0] != 0x01u) {
        platform::debug_log("[XCloud360] Invalid runtime ABI: expected Xenon PPC big-endian.\n");
        return 2;
    }

    platform::debug_log("[XCloud360] Native Xenon PowerPC / 32-bit / big-endian.\n");
    App app;
    if (!app.initialize()) {
        platform::debug_log("[XCloud360] Failed to initialize\n");
        return 1;
    }
    const int result = app.run();
    app.shutdown();
    return result;
}
