#include "xcloud_control.h"
#include "json_fields.h"
#include <stdio.h>
#include <string.h>

namespace xcloud_control {
namespace {
bool uuid(const char* id) {
    if(!id || strlen(id)!=36) return false;
    for(unsigned int i=0;i<36;++i) {
        const char c=id[i];
        if(i==8 || i==13 || i==18 || i==23) {
            if(c!='-')return false;
        } else if(!((c>='0'&&c<='9')||
                    (c>='a'&&c<='f')||(c>='A'&&c<='F'))) return false;
    }
    return true;
}
bool done(int length,size_t capacity) {
    return length>0 && (size_t)length<capacity;
}
bool quote_nested(char* out,size_t cap,const char* original) {
    if(!out||!original||cap==0)return false;
    size_t used=0;
    for(size_t i=0;original[i];++i) {
        const unsigned char ch=(unsigned char)original[i];
        if(ch<0x20 || ch>0x7e)return false;
        if(ch=='"' || ch=='\\') {
            if(used+2>=cap)return false;
            out[used++]='\\';
        } else if(used+1>=cap)return false;
        out[used++]=(char)ch;
    }
    out[used]=0;
    return true;
}
}
bool handshake(char* out,size_t cap,const char* id) {
    if(!out || !uuid(id) || cap<128)return false;
    const int n=sprintf_s(out,cap,
        "{\"type\":\"Handshake\",\"version\":\"messageV1\","
        "\"id\":\"%s\",\"cv\":\"0\"}",id);
    return done(n,cap);
}
bool handshake_ack(const char* json,size_t len) {
    char type[48];
    return json_fields::string(json,len,"type",type,sizeof(type)) &&
           strcmp(type,"HandshakeAck")==0;
}
bool gamepad_changed(char* out,size_t cap,unsigned int index,bool added) {
    if(!out || index>=4)return false;
    const int n=sprintf_s(out,cap,
        "{\"message\":\"gamepadChanged\",\"gamepadIndex\":%u,"
        "\"wasAdded\":%s}",index,added?"true":"false");
    return done(n,cap);
}
bool resolution(char* out,size_t cap,unsigned int height) {
    if(!out || !(height==360 || height==480 || height==720))return false;
    const int n=sprintf_s(out,cap,
        "{\"message\":\"userRequestedResolutionUpdate\","
        "\"resolutionAlias\":\"%u\"}",height);
    return done(n,cap);
}
bool keyframe_request(char* out,size_t cap) {
    if(!out)return false;
    const char msg[]=
        "{\"message\":\"videoKeyframeRequested\",\"ifrRequested\":true}";
    if(cap<sizeof(msg))return false;
    memcpy(out,msg,sizeof(msg));
    return true;
}
bool device_capabilities(char* out,size_t cap,const char* id,
                         unsigned int width,unsigned int height,
                         unsigned int fps,unsigned int bitrate) {
    if(!out || !uuid(id) || width<320 || width>1280 ||
       height<240 || height>720 || fps<15 || fps>60 ||
       bitrate<256 || bitrate>10000) return false;

    // The payload's nested JSON is encoded as a JSON string. These
    // video constraints are hypothetical until a legal session is negotiated.
    char body[384];
    const int n=sprintf_s(body,sizeof(body),
        "{\"supportsCustomResolution\":true,\"supportsHevc\":false,"
        "\"supportsHdr\":false,\"supportsFps\":%u,"
        "\"maxWidth\":%u,\"maxHeight\":%u,"
        "\"maxBitrateKbps\":%u,\"video\":{"
        "\"width\":%u,\"height\":%u,\"maxWidth\":%u,"
        "\"maxHeight\":%u,\"maxBitrateKbps\":%u}}",
        fps,width,height,bitrate,width,height,width,height,bitrate);
    if(!done(n,sizeof(body)))return false;
    char escaped[2*sizeof(body)];
    if(!quote_nested(escaped,sizeof(escaped),body))return false;
    const int written=sprintf_s(out,cap,
        "{\"type\":\"Message\",\"content\":\"%s\","
        "\"id\":\"%s\",\"target\":"
        "\"/streaming/characteristics/clientdevicecapabilities\","
        "\"cv\":\"\"}",escaped,id);
    return done(written,cap);
}
}
