#include "protocol_selftest.h"
#include "http_wire.h"
#include "json_fields.h"
#include "oauth_device.h"
#include "rtp_packet.h"
#include "h264_nal.h"
#include "xcloud_input.h"
#include "xcloud_control.h"
#include "xcloud_bridge.h"
#include "xbox_device_flow.h"
#include "xbox_live_auth.h"
#include "xbox_live_exchange.h"
#include "h264_rtp_receiver.h"
#include "xboxtls/sha384.h"
#include "xboxtls/tls13.h"
#include <xtl.h>
#include <string.h>

bool protocol_selftest() {
    // Offline test data only. Never makes a network request or sends an
    // authentication token. The XEX runs this once during initialization.
    // FIPS 180-4 SHA-384 vectors: detects big/little-endian and 64-bit
    // arithmetic errors that can invalidate Microsoft RSA/SHA-384 chains.
    static const unsigned char sha384_abc[48]={
        0xcb,0x00,0x75,0x3f,0x45,0xa3,0x5e,0x8b,0xb5,0xa0,0x3d,0x69,
        0x9a,0xc6,0x50,0x07,0x27,0x2c,0x32,0xab,0x0e,0xde,0xd1,0x63,
        0x1a,0x8b,0x60,0x5a,0x43,0xff,0x5b,0xed,0x80,0x86,0x07,0x2b,
        0xa1,0xe7,0xcc,0x23,0x58,0xba,0xec,0xa1,0x34,0xc8,0x25,0xa7
    };
    static const unsigned char sha384_empty[48]={
        0x38,0xb0,0x60,0xa7,0x51,0xac,0x96,0x38,0x4c,0xd9,0x32,0x7e,
        0xb1,0xb1,0xe3,0x6a,0x21,0xfd,0xb7,0x11,0x14,0xbe,0x07,0x43,
        0x4c,0x0c,0xc7,0xbf,0x63,0xf6,0xe1,0xda,0x27,0x4e,0xde,0xbf,
        0xe7,0x6f,0x65,0xfb,0xd5,0x1a,0xd2,0xf1,0x48,0x98,0xb9,0x5b
    };
    unsigned char sha384_digest[48];
    const char abc[]="abc";
    xboxtls::sha384(xboxtls::ByteSpan((const unsigned char*)abc,3),sha384_digest);
    if(memcmp(sha384_digest,sha384_abc,48)!=0)return false;
    xboxtls::sha384(xboxtls::ByteSpan((const unsigned char*)abc,0),sha384_digest);
    if(memcmp(sha384_digest,sha384_empty,48)!=0)return false;
    // RFC 8446 PSK-less TLS 1.3 early secret: HKDF-Extract(0^32, 0^32)
    static const unsigned char early_secret_expected[32]={
        0x33,0xad,0x0a,0x1c,0x60,0x7e,0xc0,0x3b,
        0x09,0xe6,0xcd,0x98,0x93,0x68,0x0c,0xe2,
        0x10,0xad,0xf3,0x00,0xaa,0x1f,0x26,0x60,
        0xe1,0xb2,0x2e,0x10,0xf1,0x70,0xf9,0x2a
    };
    xboxtls::KeyScheduleSha256 schedule;
    if(schedule.derive_early_secret(xboxtls::ByteSpan())!=xboxtls::XT_OK ||
       memcmp(schedule.early_secret,early_secret_expected,32)!=0)
        return false;

    char request[512];
    size_t written=0;
    if(!http_wire::make_request("GET","example.com","/",NULL,NULL,0,
                                request,sizeof(request),&written) ||
       written<16 || memcmp(request,"GET / HTTP/1.1",14)!=0)
        return false;

    // Strict JSON Xbox headers: bounded, format-only, no real traffic.
    const char localJson[]="{\"LocalTest\":\"true\"}";
    if(!http_wire::make_xbox_json_request(
            "user.auth.xboxlive.com","/user/authenticate",
            localJson,sizeof(localJson)-1,request,sizeof(request),
            &written) ||
       !strstr(request,"x-xbl-contract-version: 1\r\n") ||
       !strstr(request,"Content-Type: application/json\r\n") ||
       !strstr(request,localJson))
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

    if(!xbox_device_flow::offline_selftest()) return false;
    if(!xbox_live_auth::offline_selftest()) return false;
    if(!xbox_live_exchange::offline_selftest()) return false;
    if(!xcloud_bridge::offline_selftest()) return false;

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
