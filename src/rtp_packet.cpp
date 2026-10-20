#include "rtp_packet.h"

namespace rtp {
namespace {
unsigned short be16(const unsigned char* p) {
    return (unsigned short)(((unsigned int)p[0]<<8)|p[1]);
}
unsigned int be32(const unsigned char* p) {
    return ((unsigned int)p[0]<<24)|((unsigned int)p[1]<<16)|
           ((unsigned int)p[2]<<8)|(unsigned int)p[3];
}
}
bool parse(const unsigned char* data,size_t len,Packet* out) {
    if(!data||!out||len<12||len>65535)return false;
    if((data[0]>>6)!=2)return false;
    const unsigned int cc=data[0]&15;
    const bool hasPadding=(data[0]&0x20)!=0;
    const bool hasExtension=(data[0]&0x10)!=0;
    size_t offset=12+cc*4;
    if(offset>len)return false;
    if(hasExtension) {
        if(offset+4>len)return false;
        const unsigned int extensionWords=be16(data+offset+2);
        const size_t extensionBytes=(size_t)extensionWords*4;
        if(extensionBytes>len-offset-4)return false;
        offset+=4+extensionBytes;
    }
    size_t pad=0;
    if(hasPadding) {
        pad=data[len-1];
        if(!pad||pad>len-offset)return false;
    }
    Packet packet;
    packet.payload_type=data[1]&0x7f;
    packet.marker=(data[1]&0x80)!=0;
    packet.sequence=be16(data+2);
    packet.timestamp=be32(data+4);
    packet.ssrc=be32(data+8);
    packet.payload_offset=offset;
    packet.payload_size=len-offset-pad;
    *out=packet;
    return true;
}
}
