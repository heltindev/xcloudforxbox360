#include "network_ready.h"
#include <stdio.h>
using namespace network_ready;
#define CHECK(x) {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}
struct Fixture {unsigned int now,started,ready_at,reads;bool unsupported,trouble;};
static unsigned int tick(void* p){return ((Fixture*)p)->now;}
static void pause(void* p,unsigned int ms){((Fixture*)p)->now+=ms;}
static void read(void* p,Sample* out) {
    Fixture* f=(Fixture*)p;++f->reads;
    out->link_flags=0; // Do not reject Xenia/Wi-Fi because Ethernet bits are absent.
    out->local_ip=0;
    out->address_flags=2;
    if(f->unsupported){out->address_flags=ADDR_NONE;return;}
    if(f->trouble){out->address_flags=ADDR_TROUBLE;return;}
    if(f->now-f->started>=f->ready_at){
        out->address_flags=ADDR_DHCP|0x20|0x40;out->local_ip=0x0101a8c0u;
    }
}
int main() {
    Fixture f={100,100,350,0,false,false};
    Result r=wait(&f,read,tick,pause,1000);
    CHECK(r.state==READY && r.waited_ms==350 && f.reads==8);
    f.now=0xffffff00u;f.started=f.now;f.ready_at=400;f.reads=0;
    r=wait(&f,read,tick,pause,500);
    CHECK(r.state==READY && r.waited_ms==400);
    f.now=0;f.started=0;f.ready_at=501;f.reads=0;
    r=wait(&f,read,tick,pause,500);
    CHECK(r.state==ADDRESS_TIMEOUT && r.waited_ms==500);
    f.now=0;f.ready_at=500;f.reads=0;
    r=wait(&f,read,tick,pause,500);
    CHECK(r.state==READY && r.waited_ms==500);
    f.unsupported=true;f.reads=0;
    r=wait(&f,read,tick,pause,10000);
    CHECK(r.state==UNAVAILABLE && r.waited_ms==0 && f.reads==1);
    f.unsupported=false;f.trouble=true;
    r=wait(&f,read,tick,pause,10000);
    CHECK(r.state==TROUBLESHOOT && r.waited_ms==0);
    Sample bad={1,ADDR_DHCP,0};
    CHECK(classify(bad)==WAITING);
    bad.local_ip=0xffffffffu;CHECK(classify(bad)==WAITING);
    bad.local_ip=1;CHECK(classify(bad)==READY);
    CHECK(wait(NULL,NULL,NULL,NULL,100).state==UNAVAILABLE);
    puts("PASS: delayed DHCP, timeout boundaries, 32-bit tick rollover, unavailable XNet, trouble flags and valid-IP requirements.");
    return 0;
}
