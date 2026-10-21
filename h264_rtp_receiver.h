#pragma once
#include <stddef.h>

// Portable RTP/H.264 packetization-mode-1 receiver, inspired by GR33N's
// layered design. Independent code, no PS3-specific API dependencies.
//
// INPUT MUST ALREADY BE DECRYPTED AND AUTHENTICATED by SRTP; the project
// does not yet have WebRTC, SRTP, a H.264 decoder or media output.
// Supports RFC 6184: Single-NAL, STAP-A and FU-A in sequence order.
// A real streaming client still needs a jitter/reordering buffer + NACK.
namespace h264_rtp {
enum { ACCESS_UNIT_CAPACITY=512*1024 };
enum PushResult { PACKET_INVALID=-1, WAITING=0, ACCESS_UNIT=1, NEED_KEYFRAME=2 };
struct Frame {
    const unsigned char* annex_b;
    size_t size;
    unsigned int rtp_timestamp;
    bool contains_idr;
};
struct Stats {
    unsigned int accepted;
    unsigned int completed;
    unsigned int dropped;
    unsigned int invalid;
};
class Receiver {
public:
    Receiver();
    void reset();
    PushResult push(const unsigned char* rtp_packet, size_t rtp_bytes,
                    Frame* completed);
    bool waiting_for_idr() const { return need_idr_; }
    const Stats& stats() const { return stats_; }
private:
    bool append(const unsigned char* bytes, size_t amount);
    bool append_nal(const unsigned char* bytes, size_t amount);
    void abandon_frame();
    bool active_;
    bool fragmented_;
    bool need_idr_;
    bool has_idr_;
    bool have_seq_;
    bool have_ssrc_;
    unsigned short next_seq_;
    unsigned int ssrc_;
    unsigned int timestamp_;
    size_t used_;
    Stats stats_;
    unsigned char data_[ACCESS_UNIT_CAPACITY];
};
}
