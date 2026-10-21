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
// This is an entirely LOCAL regression; it never contacts Microsoft/xCloud.
// Calling set_session_ready(true) below simulates an authorized WebRTC peer
// ONLY inside this test. Production code must obtain its own authenticated peer.
namespace {
struct OfflineSink {
    bool accept;
    unsigned int expected_sequence;
    unsigned int input_count;
    unsigned int text_count;
    unsigned int frame_count;
};
bool local_text(void* p,unsigned short sid,const char* json,size_t bytes) {
    OfflineSink* sink=(OfflineSink*)p;
    if(!sink || !sink->accept || !json || !bytes)return false;
    if(sid!=CONTROL_CHANNEL && sid!=MESSAGE_CHANNEL)return false;
    if(sid==MESSAGE_CHANNEL && !strstr(json,"\"Handshake\""))return false;
    if(sid==CONTROL_CHANNEL && !strstr(json,"videoKeyframeRequested"))
        return false;
    ++sink->text_count;
    return true;
}
bool local_input(void* p,unsigned short sid,const unsigned char* data,
                 size_t bytes) {
    OfflineSink* sink=(OfflineSink*)p;
    if(!sink || !sink->accept || !data || sid!=INPUT_CHANNEL ||
       (bytes!=xcloud_input::METADATA_LENGTH &&
        bytes!=xcloud_input::GAMEPAD_LENGTH))return false;
    const unsigned int seq=(unsigned int)data[2] |
       ((unsigned int)data[3]<<8) | ((unsigned int)data[4]<<16) |
       ((unsigned int)data[5]<<24);
    if(seq!=sink->expected_sequence)return false;
    ++sink->expected_sequence;
    ++sink->input_count;
    return true;
}
void local_frame(void* p,const unsigned char* nal,size_t bytes,
                 unsigned int timestamp) {
    OfflineSink* sink=(OfflineSink*)p;
    static const unsigned char expected[]={0,0,0,1,0x65,0x88};
    if(sink && bytes==sizeof(expected) && timestamp==1 &&
       nal && memcmp(nal,expected,sizeof(expected))==0)
        ++sink->frame_count;
}
}
bool offline_selftest() {
    static Bridge peer; // fixed RTP H.264 buffer >512KB; keep off stack
    peer.reset();
    OfflineSink sink={true,1,0,0,0};
    Callbacks callbacks={&sink,local_input,local_text,local_frame};
    peer.set_callbacks(callbacks);
    const char id[]="00000000-0000-4000-8000-000000000001";
    const char ack[]="{\"type\":\"HandshakeAck\",\"cv\":\"0\"}";
    const char invalid[]="{\"type\":\"Error\",\"cv\":\"0\"}";
    if(peer.begin_handshake(id) || peer.ready_for_input() ||
       peer.handle_text(MESSAGE_CHANNEL,ack,sizeof(ack)-1))return false;
    peer.set_session_ready(true);
    if(!peer.begin_handshake(id) || sink.text_count!=1 ||
       peer.ready_for_input() ||
       peer.handle_text(MESSAGE_CHANNEL,invalid,sizeof(invalid)-1) ||
       peer.handle_text(CONTROL_CHANNEL,ack,sizeof(ack)-1) ||
       !peer.handle_text(MESSAGE_CHANNEL,ack,sizeof(ack)-1) ||
       !peer.ready_for_input())return false;
    controller_input::Snapshot pad={0};
    sink.accept=false;
    if(peer.send_gamepad(pad,16.0,0) || sink.input_count!=0)return false;
    sink.accept=true;
    if(!peer.send_gamepad(pad,16.0,0) ||
       !peer.send_metadata() || sink.input_count!=2 ||
       sink.expected_sequence!=3 ||
       !peer.request_keyframe() || sink.text_count!=2)return false;
    static const unsigned char idr[]={
        0x80,0xe0,0,1,0,0,0,1,0,0,0,1,0x65,0x88
    };
    if(peer.receive_video(idr,sizeof(idr))!=h264_rtp::ACCESS_UNIT ||
       sink.frame_count!=1)return false;
    peer.set_session_ready(false);
    if(peer.ready_for_input() || peer.send_metadata() ||
       peer.receive_video(idr,sizeof(idr))!=h264_rtp::PACKET_INVALID)
        return false;
    peer.reset();
    return true;
}
}
