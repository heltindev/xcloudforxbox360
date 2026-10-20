#pragma once
#include <stddef.h>

// Parser for already authenticated/decrypted RTP packets.
// Does not implement ICE, DTLS, SRTP, jitter buffer or codecs.
namespace rtp {
struct Packet {
    unsigned char payload_type;
    bool marker;
    unsigned short sequence;
    unsigned int timestamp;
    unsigned int ssrc;
    size_t payload_offset;
    size_t payload_size;
};
bool parse(const unsigned char* buffer,size_t length,Packet* out);
}
