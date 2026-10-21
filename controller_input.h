#pragma once

// Native XInput snapshot for eventual xCloud data-channel input encoding.
// Captures controller state on the actual Xbox 360, no cloud traffic.
namespace controller_input {
struct Snapshot {
    unsigned int port;
    unsigned int packet_number;
    unsigned short buttons;
    unsigned char left_trigger;
    unsigned char right_trigger;
    short lx,ly,rx,ry;
};
bool read_port(unsigned int port,Snapshot* snapshot);
bool read_first(Snapshot* snapshot);
// -32768..32767 -> -1..1 normalized, 0 around the deadzone.
float normalize_axis(short raw,unsigned int deadzone);
}
