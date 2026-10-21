#pragma once
#include <stddef.h>
#include "controller_input.h"
#include "xcloud_input.h"
#include "h264_rtp_receiver.h"

// Adaptation of the GR33N high-level architecture to native Xbox 360.
// Decouples data-channel messages, controller packets, and decrypted video
// from any PS3 runtime. All callbacks must be supplied by a FUTURE verified,
// authorized WebRTC peer. No network, sign-in, video decoder or DLL is included.
namespace xcloud_bridge {
// Offline, synthetic end-to-end data-channel and RTP adapter regression.
// This must NEVER be reported as proof of a live xCloud stream.
bool offline_selftest();
enum Channel {
    CONTROL_CHANNEL=0,
    INPUT_CHANNEL=2,
    MESSAGE_CHANNEL=4,
    CHAT_CHANNEL=6
};
typedef bool (*BinarySender)(void* context, unsigned short sid,
                             const unsigned char* bytes,size_t count);
typedef bool (*TextSender)(void* context, unsigned short sid,
                           const char* bytes,size_t count);
typedef void (*AccessUnitSink)(void* context,const unsigned char* annex_b,
                               size_t count,unsigned int timestamp);
struct Callbacks {
    void* context;
    BinarySender binary;
    TextSender text;
    AccessUnitSink frame;
};
class Bridge {
public:
    Bridge();
    void reset();
    void set_callbacks(const Callbacks& callbacks);
    // Only a real, certificate-verified, authorized session should call this.
    void set_session_ready(bool authenticated_and_channels_open);
    bool begin_handshake(const char* session_generated_id);
    bool handle_text(unsigned short channel,const char* payload,size_t length);
    bool send_metadata();
    bool send_gamepad(const controller_input::Snapshot& state,
                      double session_milliseconds,unsigned int deadzone);
    bool request_keyframe();
    h264_rtp::PushResult receive_video(const unsigned char* decrypted_rtp,
                                       size_t length);
    bool ready_for_input() const;
    bool needs_video_keyframe() const;
    const h264_rtp::Stats& video_stats() const;
private:
    Callbacks callbacks_;
    bool secure_session_ready_;
    bool handshake_confirmed_;
    xcloud_input::Encoder encoder_;
    h264_rtp::Receiver video_;
};
}
