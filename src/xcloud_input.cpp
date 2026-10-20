#include "xcloud_input.h"
#include <xtl.h>
#include <string.h>

namespace xcloud_input {
namespace {
void put_le16(unsigned char* dest, unsigned short value) {
    dest[0]=(unsigned char)value;
    dest[1]=(unsigned char)(value>>8);
}
void put_le32(unsigned char* dest, unsigned int value) {
    dest[0]=(unsigned char)value;
    dest[1]=(unsigned char)(value>>8);
    dest[2]=(unsigned char)(value>>16);
    dest[3]=(unsigned char)(value>>24);
}
void put_le_double(unsigned char* dest, double value) {
    // IEEE754 double is serialized explicitly little-endian; the Xenon
    // PowerPC target itself is big-endian.
    unsigned char native[sizeof(double)];
    if(sizeof(double)!=8) return;
    memcpy(native,&value,8);
    const unsigned short marker=0x0102u;
    const bool big_endian=(*(const unsigned char*)&marker)==0x01;
    for(unsigned int i=0;i<8;++i)
        dest[i]=native[big_endian?7-i:i];
}
short scaled_axis(short value,unsigned int deadzone) {
    int n=(int)value;
    if(deadzone>32767u)deadzone=32767u;
    const int absval=n<0?-n:n;
    if(absval<=(int)deadzone) return 0;
    const int range=32767-(int)deadzone;
    if(range<=0)return n<0?(short)-32767:(short)32767;
    int scaled=((absval-(int)deadzone)*32767)/range;
    if(scaled>32767)scaled=32767;
    return (short)(n<0?-scaled:scaled);
}
}
unsigned short buttons_from_xinput(unsigned short b) {
    unsigned short c=0;
    if(b & XINPUT_GAMEPAD_START)c|=CLOUD_MENU;
    if(b & XINPUT_GAMEPAD_BACK)c|=CLOUD_VIEW;
    if(b & XINPUT_GAMEPAD_A)c|=CLOUD_A;
    if(b & XINPUT_GAMEPAD_B)c|=CLOUD_B;
    if(b & XINPUT_GAMEPAD_X)c|=CLOUD_X;
    if(b & XINPUT_GAMEPAD_Y)c|=CLOUD_Y;
    if(b & XINPUT_GAMEPAD_DPAD_UP)c|=CLOUD_UP;
    if(b & XINPUT_GAMEPAD_DPAD_DOWN)c|=CLOUD_DOWN;
    if(b & XINPUT_GAMEPAD_DPAD_LEFT)c|=CLOUD_LEFT;
    if(b & XINPUT_GAMEPAD_DPAD_RIGHT)c|=CLOUD_RIGHT;
    if(b & XINPUT_GAMEPAD_LEFT_SHOULDER)c|=CLOUD_LB;
    if(b & XINPUT_GAMEPAD_RIGHT_SHOULDER)c|=CLOUD_RB;
    if(b & XINPUT_GAMEPAD_LEFT_THUMB)c|=CLOUD_LS;
    if(b & XINPUT_GAMEPAD_RIGHT_THUMB)c|=CLOUD_RS;
    // Guide/Nexus is reserved and is not forwarded by this adapter.
    return c;
}
bool Encoder::metadata(unsigned char* out,size_t cap,size_t* written) {
    if(written)*written=0;
    if(!out || !written || cap<METADATA_LENGTH) return false;
    // Report type 8, sequence (shared with gamepad reports), time=0,
    // maximum touch contacts=0.
    memset(out,0,METADATA_LENGTH);
    put_le16(out,8);
    put_le32(out+2,++sequence);
    *written=METADATA_LENGTH;
    return true;
}
bool Encoder::gamepad(const controller_input::Snapshot& s,
                      double elapsed_ms,unsigned int deadzone,
                      unsigned char* out,size_t cap,size_t* written) {
    if(written)*written=0;
    if(!out || !written || cap<GAMEPAD_LENGTH ||
       s.port>=4 || elapsed_ms!=elapsed_ms ||
       elapsed_ms<0.0 || elapsed_ms>86400000.0)
        return false;
    memset(out,0,GAMEPAD_LENGTH);
    put_le16(out,2); // gamepad report
    put_le32(out+2,++sequence);
    put_le_double(out+6,elapsed_ms);
    out[14]=1; // controller count
    out[15]=(unsigned char)s.port;
    put_le16(out+16,buttons_from_xinput(s.buttons));
    put_le16(out+18,(unsigned short)scaled_axis(s.lx,deadzone));
    // Cloud wire protocol uses opposite vertical axis convention.
    const short ly=scaled_axis(s.ly,deadzone);
    const short ry=scaled_axis(s.ry,deadzone);
    put_le16(out+20,(unsigned short)(ly==-32768?32767:-ly));
    put_le16(out+22,(unsigned short)scaled_axis(s.rx,deadzone));
    put_le16(out+24,(unsigned short)(ry==-32768?32767:-ry));
    put_le16(out+26,(unsigned short)((unsigned int)s.left_trigger*257u));
    put_le16(out+28,(unsigned short)((unsigned int)s.right_trigger*257u));
    put_le32(out+30,1u); // physical gamepad instance
    out[37]=1; // terminal four-byte field is 00 00 00 01
    *written=GAMEPAD_LENGTH;
    return true;
}
}
