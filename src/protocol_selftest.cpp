#include "protocol_selftest.h"
#include "http_wire.h"
#include "json_fields.h"
#include "oauth_device.h"
#include "rtp_packet.h"
#include "h264_nal.h"
#include <string.h>

bool protocol_selftest() {
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
    const bool oauthOkay=auth.expires_in==900 && auth.interval==5;
    oauth_device::clear(&auth);
    if(!oauthOkay)return false;

    const unsigned char rtpData[]={
        0x80,0x60,0x12,0x34,0,0,0,1,0,0,0,2,0x55
    };
    rtp::Packet packet;
    if(!rtp::parse(rtpData,sizeof(rtpData),&packet) ||
       packet.sequence!=0x1234 || packet.payload_size!=1)
        return false;

    const unsigned char nalBytes[]={
        0,0,0,1,0x65,0x88,0x00,0,0,1,0x41,0x99
    };
    size_t cursor=0;
    h264::NalUnit nal;
    if(!h264::next_nal(nalBytes,sizeof(nalBytes),&cursor,&nal) ||
       nal.nal_type!=5)
        return false;
    if(!h264::next_nal(nalBytes,sizeof(nalBytes),&cursor,&nal) ||
       nal.nal_type!=1)
        return false;
    return true;
}
