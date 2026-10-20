#include "protocol_selftest.h"
#include "http_wire.h"
#include "json_fields.h"
#include "oauth_device.h"
#include "rtp_packet.h"
#include "h264_nal.h"
#include "xcloud_input.h"
#include "xcloud_control.h"
#include "h264_rtp_receiver.h"
#include <xtl.h>
#include <string.h>

bool protocol_selftest() {
    // Offline test data only. Never makes a network request or sends an
    // authentication token. The XEX runs this once during initialization.
    char request[512];
    size_t written=0;
    if(!http_wire::make_request("GET","example.com","/",NULL,NULL,0,
                                request,sizeof(request),&written) ||
       written<16 || memcmp(request,"GET / HTTP/1.1",14)!=0)
        return false;

    const char exampleResponse[]="HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
    http_wire::Response parsed;
    if(!http_wire::parse_response(exampleResponse,sizeof(exampleResponse)-1,
                                  &parsed) ||
       parsed.status!=200 || !parsed.has_length || parsed.content_length!=2)
        return false;

    const char json[]="{\"verification_uri\":\"https://www.microsoft.com/link\","
                      "\"device_code\":\"testdevicecode\","
                      "\"user_code\":\"TEST-CODE\","
                      "\"interval\":5,\"expires_in\":900}";
    oauth_device::DeviceAuthorization auth;
    if(!oauth_device::read_device_response(json,sizeof(json)-1,&auth))
        return false;
    const bool oauth_ok=auth.expires_in==900 && auth.interval==5;
    oauth_device::clear(&auth);
    if(!oauth_ok)return false;

    const unsigned char rtp_data[]={
        0x80,0x60,0x12,0x34,0,0,0,1,0,0,0,2,0x55
    };
    rtp::Packet packet;
    if(!rtp::parse(rtp_data,sizeof(rtp_data),&packet) ||
       packet.sequence!=0x1234 || packet.payload_size!=1)
        return false;

    const unsigned char nal_bytes[]={
        0,0,0,1,0x65,0x88,0x00,0,0,1,0x41,0x99
    };
    size_t cursor=0;
    h264::NalUnit nal;
    if(!h264::next_nal(nal_bytes,sizeof(nal_bytes),&cursor,&nal) ||
       nal.nal_type!=5)
        return false;
    if(!h264::next_nal(nal_bytes,sizeof(nal_bytes),&cursor,&nal) ||
       nal.nal_type!=1)
        return false;

    // GR33N-informed Xbox cloud input wire format, encoded on Xenon
    // without copying big-endian host integer fields into network packets.
    xcloud_input::Encoder encoder;
    unsigned char binary[64];
    if(!encoder.metadata(binary,sizeof(binary),&written) ||
       written!=xcloud_input::METADATA_LENGTH ||
       binary[0]!=8 || binary[1]!=0 || binary[2]!=1 ||
       binary[14]!=0)
        return false;
    controller_input::Snapshot state;
    memset(&state,0,sizeof(state));
    state.buttons=(unsigned short)(XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_START);
    state.left_trigger=255;
    state.lx=32767;
    if(!encoder.gamepad(state,16.0,0,binary,sizeof(binary),&written) ||
       written!=xcloud_input::GAMEPAD_LENGTH ||
       binary[0]!=2 || binary[1]!=0 || binary[2]!=2 ||
       binary[14]!=1 || binary[15]!=0 ||
       binary[16]!=(xcloud_input::CLOUD_A | xcloud_input::CLOUD_MENU) ||
       binary[18]!=0xff || binary[19]!=0x7f ||
       binary[26]!=0xff || binary[27]!=0xff ||
       binary[34]!=0 || binary[35]!=0 ||
       binary[36]!=0 || binary[37]!=1)
        return false;

    // A protocol message must be fully parsed; a raw substring "HandshakeAck"
    // in an unrelated field is insufficient.
    char message[1024];
    const char sample_id[]="00000000-0000-4000-8000-000000000001";
    if(!xcloud_control::handshake(message,sizeof(message),sample_id) ||
       strstr(message,"\"messageV1\"")==NULL)
        return false;
    const char ack[]="{\"type\":\"HandshakeAck\",\"cv\":\"0\"}";
    const char false_ack[]="{\"other\":\"HandshakeAck\",\"type\":\"Error\"}";
    if(!xcloud_control::handshake_ack(ack,sizeof(ack)-1) ||
       xcloud_control::handshake_ack(false_ack,sizeof(false_ack)-1))
        return false;
    if(!xcloud_control::gamepad_changed(message,sizeof(message),0,true) ||
       strstr(message,"gamepadChanged")==NULL ||
       !xcloud_control::resolution(message,sizeof(message),720) ||
       !xcloud_control::keyframe_request(message,sizeof(message)) ||
       !xcloud_control::device_capabilities(message,sizeof(message),sample_id,
                                           1280,720,30,3500) ||
       strstr(message,"clientdevicecapabilities")==NULL)
        return false;

    // Keep the large 512-KB AU buffer outside the startup stack.
    static h264_rtp::Receiver video;
    video.reset();
    h264_rtp::Frame decoded;
    // The example is a made-up IDR NAL; it is only depacketized, NEVER decoded.
    const unsigned char idr[]={
        0x80,0xe0,0,1,0,0,0,1,0,0,0,1,0x65,0x88
    };
    if(video.push(idr,sizeof(idr),&decoded)!=h264_rtp::ACCESS_UNIT ||
       decoded.size!=6 ||
       memcmp(decoded.annex_b,"\x00\x00\x00\x01\x65\x88",6)!=0 ||
       !decoded.contains_idr)
        return false;

    // FU-A: reconstruct an IDR spanning two RTP packets.
    const unsigned char start_fu[]={
        0x80,0x60,0,2,0,0,0,2,0,0,0,1,0x7c,0x85,0xaa
    };
    const unsigned char end_fu[]={
        0x80,0xe0,0,3,0,0,0,2,0,0,0,1,0x7c,0x45,0xbb
    };
    if(video.push(start_fu,sizeof(start_fu),&decoded)!=h264_rtp::WAITING ||
       video.push(end_fu,sizeof(end_fu),&decoded)!=h264_rtp::ACCESS_UNIT ||
       decoded.size!=7 ||
       memcmp(decoded.annex_b,"\x00\x00\x00\x01\x65\xaa\xbb",7)!=0)
        return false;

    // A missing RTP sequence must force waiting for a new keyframe.
    const unsigned char lost_p[]={
        0x80,0xe0,0,5,0,0,0,3,0,0,0,1,0x41,0x10
    };
    if(video.push(lost_p,sizeof(lost_p),&decoded)!=h264_rtp::NEED_KEYFRAME ||
       !video.waiting_for_idr())
        return false;

    // STAP-A with SPS and IDR in the same access unit.
    const unsigned char stap[]={
        0x80,0xe0,0,6,0,0,0,4,0,0,0,1,
        0x78,0,2,0x67,0x42,0,2,0x65,0x88
    };
    if(video.push(stap,sizeof(stap),&decoded)!=h264_rtp::ACCESS_UNIT ||
       decoded.size!=12 || !decoded.contains_idr)
        return false;
    return true;
}
