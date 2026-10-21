#pragma once
// XDK address flags are checked against the SDK in xbox_network_ready.h.
// This policy is portable so delayed DHCP and tick rollover can be tested.
namespace network_ready {
enum State { WAITING=0, READY=1, ADDRESS_TIMEOUT=2, UNAVAILABLE=3, TROUBLESHOOT=4 };
enum Flags { ADDR_NONE=1, ADDR_STATIC=4, ADDR_DHCP=8, ADDR_PPPOE=16, ADDR_TROUBLE=0x8000 };
struct Sample {unsigned int link_flags,address_flags,local_ip;};
struct Result {Sample sample;State state;unsigned int waited_ms;};
typedef void (*Reader)(void*,Sample*);
typedef unsigned int (*Clock)(void*);
typedef void (*Pause)(void*,unsigned int);
inline State classify(const Sample& s) {
    if(s.address_flags&ADDR_TROUBLE)return TROUBLESHOOT;
    if(s.address_flags&ADDR_NONE)return UNAVAILABLE;
    const unsigned int assignment=ADDR_STATIC|ADDR_DHCP|ADDR_PPPOE;
    if((s.address_flags&assignment) && s.local_ip && s.local_ip!=0xffffffffu)
        return READY;
    return WAITING;
}
inline const char* text(State state) {
    switch(state) {
    case READY:return "IP READY";
    case ADDRESS_TIMEOUT:return "IP WAIT TIMEOUT";
    case UNAVAILABLE:return "XNET ADDRESS UNAVAILABLE";
    case TROUBLESHOOT:return "XNET CONFIG TROUBLE";
    default:return "WAITING FOR IP";
    }
}
inline Result wait(void* context,Reader read,Clock clock,Pause pause,
                   unsigned int limit_ms) {
    Result out={{0,0,0},UNAVAILABLE,0};
    if(!read || !clock || !pause)return out;
    if(limit_ms>30000u)limit_ms=30000u;
    const unsigned int begin=clock(context);
    for(;;) {
        read(context,&out.sample);
        out.waited_ms=clock(context)-begin;
        out.state=classify(out.sample);
        if(out.state!=WAITING)return out;
        if(out.waited_ms>=limit_ms) {out.state=ADDRESS_TIMEOUT;return out;}
        unsigned int remaining=limit_ms-out.waited_ms;
        pause(context,remaining<50u?remaining:50u);
    }
}
} // namespace network_ready
