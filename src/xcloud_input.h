#pragma once
#include <stddef.h>
#include "controller_input.h"

// Native Xenon PowerPC adapter for the binary xCloud input-channel layout.
// The wire format is based on interoperability study of GR33N / green-nx.
// Independent implementation; no PS3 headers, runtime or libraries.
// IMPORTANT: these packets can only be sent over an authenticated,
// negotiated WebRTC data channel; that transport does not yet exist.
namespace xcloud_input {
enum { METADATA_LENGTH=15, GAMEPAD_LENGTH=38 };
enum CloudButton {
    CLOUD_NEXUS=2, CLOUD_MENU=4, CLOUD_VIEW=8,
    CLOUD_A=16, CLOUD_B=32, CLOUD_X=64, CLOUD_Y=128,
    CLOUD_UP=256, CLOUD_DOWN=512, CLOUD_LEFT=1024,
    CLOUD_RIGHT=2048, CLOUD_LB=4096, CLOUD_RB=8192,
    CLOUD_LS=16384, CLOUD_RS=32768
};
struct Encoder {
    unsigned int sequence;   // Single increasing sequence for all reports.
    Encoder() : sequence(0) {}
    void reset() { sequence=0; }
    bool metadata(unsigned char* out, size_t capacity, size_t* written);
    bool gamepad(const controller_input::Snapshot& state,
                 double elapsed_ms, unsigned int deadzone,
                 unsigned char* out, size_t capacity, size_t* written);
};
unsigned short buttons_from_xinput(unsigned short xbox_buttons);
}
