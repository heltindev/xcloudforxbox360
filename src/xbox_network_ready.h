#pragma once
#include "network_ready.h"
#include <xtl.h>
#include <winsockx.h>
#include "xkelib.h"
// Keep address acquisition on the SAME caller as the SYSAPP sockets.
namespace xbox_network_ready {
typedef char FlagNone[(XNET_GET_XNADDR_NONE==network_ready::ADDR_NONE)?1:-1];
typedef char FlagStatic[(XNET_GET_XNADDR_STATIC==network_ready::ADDR_STATIC)?1:-1];
typedef char FlagDhcp[(XNET_GET_XNADDR_DHCP==network_ready::ADDR_DHCP)?1:-1];
typedef char FlagPppoe[(XNET_GET_XNADDR_PPPOE==network_ready::ADDR_PPPOE)?1:-1];
typedef char FlagTrouble[(XNET_GET_XNADDR_TROUBLESHOOT==network_ready::ADDR_TROUBLE)?1:-1];
inline void sample(void*,network_ready::Sample* out) {
    XNADDR address;
    ZeroMemory(&address,sizeof(address));
    out->link_flags=(unsigned int)NetDll_XNetGetEthernetLinkStatus(XNCALLER_SYSAPP);
    out->address_flags=(unsigned int)NetDll_XNetGetTitleXnAddr(XNCALLER_SYSAPP,&address);
    out->local_ip=(unsigned int)address.ina.s_addr;
}
inline unsigned int clock(void*) {return (unsigned int)GetTickCount();}
inline void pause(void*,unsigned int ms) {Sleep(ms);}
inline network_ready::Result wait() {
    return network_ready::wait(NULL,&sample,&clock,&pause,10000u);
}
} // namespace xbox_network_ready
