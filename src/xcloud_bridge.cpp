#include "xcloud_bridge.h"
#include "xcloud_control.h"
#include <string.h>

namespace xcloud_bridge {
Bridge::Bridge() { reset(); }
void Bridge::reset() {
    memset(&callbacks_,0,sizeof(callbacks_));
    secure_session_ready_=false;
    handshake_confirmed_=false;
    encoder_.reset();
    video_.reset();
}
void Bridge::set_callbacks(const Callbacks& cb) { callbacks_=cb; }
void Bridge::set_session_ready(bool enabled) {
    secure_session_ready_=enabled;
    handshake_confirmed_=false;
    encoder_.reset();
    video_.reset();
}
bool Bridge::begin_handshake(const char* id) {
    if(!secure_session_ready_ || !callbacks_.text)return false;
    char body[256];
    if(!xcloud_control::handshake(body,sizeof(body),id))return false;
    return callbacks_.text(callbacks_.context,MESSAGE_CHANNEL,
                           body,strlen(body));
}
bool Bridge::handle_text(unsigned short channel,const char* body,size_t length) {
    if(!secure_session_ready_ || !body || !length || length>8192)
        return false;
    if(channel!=MESSAGE_CHANNEL)return false;
    if(xcloud_control::handshake_ack(body,length)) {
        handshake_confirmed_=true;
        return true;
    }
    return false;
}
bool Bridge::ready_for_input() const {
    return secure_session_ready_ && handshake_confirmed_ &&
           callbacks_.binary!=NULL;
}
bool Bridge::send_metadata() {
    if(!ready_for_input())return false;
    unsigned char bytes[xcloud_input::METADATA_LENGTH];
    size_t used=0;
    xcloud_input::Encoder next=encoder_;
    if(!next.metadata(bytes,sizeof(bytes),&used))return false;
    if(!callbacks_.binary(callbacks_.context,INPUT_CHANNEL,bytes,used))
        return false;
    encoder_=next;  // Never skip a sequence number on failed sending.
    return true;
}
bool Bridge::send_gamepad(const controller_input::Snapshot& state,
                          double time_ms,unsigned int deadzone) {
    if(!ready_for_input())return false;
    unsigned char bytes[xcloud_input::GAMEPAD_LENGTH];
    size_t used=0;
    xcloud_input::Encoder next=encoder_;
    if(!next.gamepad(state,time_ms,deadzone,bytes,sizeof(bytes),&used))
        return false;
    if(!callbacks_.binary(callbacks_.context,INPUT_CHANNEL,bytes,used))
        return false;
    encoder_=next;  // Preserve contiguous message sequence numbers.
    return true;
}
bool Bridge::request_keyframe() {
    if(!secure_session_ready_ || !handshake_confirmed_ ||
       !callbacks_.text)return false;
    char packet[128];
    if(!xcloud_control::keyframe_request(packet,sizeof(packet)))return false;
    return callbacks_.text(callbacks_.context,CONTROL_CHANNEL,
                           packet,strlen(packet));
}
h264_rtp::PushResult Bridge::receive_video(const unsigned char* data,size_t n) {
    if(!secure_session_ready_ || !callbacks_.frame)return h264_rtp::PACKET_INVALID;
    h264_rtp::Frame frame;
    const h264_rtp::PushResult result=video_.push(data,n,&frame);
    if(result==h264_rtp::ACCESS_UNIT) {
        // Buffer is only valid during this call; the future decoder queue
        // must copy it, never run blocking video decoding in the peer loop.
        callbacks_.frame(callbacks_.context,frame.annex_b,
                         frame.size,frame.rtp_timestamp);
    }
    return result;
}
bool Bridge::needs_video_keyframe() const {
    return video_.waiting_for_idr();
}
const h264_rtp::Stats& Bridge::video_stats() const {
    return video_.stats();
}
}
