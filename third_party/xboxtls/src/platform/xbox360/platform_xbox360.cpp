#include "xboxtls/xbox360_platform.h"

#if defined(_XBOX)
#include <string.h>
#include <winsockx.h>
#include "xkelib.h"
#include "syssock.h"
#include "udp_dns.h" // Reuse the Xbox 360 public DNS fallbacks.
#include "xbox_network_ready.h"
#include <stdio.h>

namespace xboxtls {

// Human-readable network diagnostics for RGH hardware and Xenia.
// All stored values are non-sensitive. No authentication data.
static XboxNetworkDiagnostic g_netdiag = {"NOT STARTED",0,0,0,0,0,0,0,0,0,0};
void xbox360_network_diagnostic(XboxNetworkDiagnostic* out) {
    if(out) *out=g_netdiag;
}
static void net_stage(const char* name,int error) {
    g_netdiag.stage=name;
    g_netdiag.error=error;
}

Error Xbox360Platform::random_bytes(MutableByteSpan out) {
    if (!out.data && out.size)
        return XT_ERR_INVALID_ARGUMENT;
    if (out.size > 0xFFFFFFFFu)
        return XT_ERR_INVALID_ARGUMENT;
    if (out.size)
        XeCryptRandom(out.data, (DWORD)out.size);
    return XT_OK;
}

xt_u64 Xbox360Platform::unix_time_seconds() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    if (u.QuadPart < 116444736000000000ULL)
        return 0;
    return (xt_u64)((u.QuadPart - 116444736000000000ULL) / 10000000ULL);
}

Error xbox360_network_startup() {
    g_netdiag.addresses=0;
    g_netdiag.link_flags=0;
    g_netdiag.xnaddr_flags=0;
    g_netdiag.local_ip=0;
    g_netdiag.address_state=network_ready::WAITING;
    g_netdiag.address_wait_ms=0;
    g_netdiag.connect_attempts=0;
    g_netdiag.peer_ip=0;
    g_netdiag.connect_ms=0;
    net_stage("XNET STARTUP",0);
    XNetStartupParams xn;
    memset(&xn, 0, sizeof(xn));
    xn.cfgSizeOfStruct = sizeof(xn);
    // The installed XDK documents this as devkit-only; secure retail XNet
    // may ignore it. RGH Internet sockets also request SO_MARKINSECURE.
    // Neither setting disables certificate/hostname checks in XboxTLS.
    xn.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    const int status=XNetStartup(&xn);
    if (status != 0) {
        net_stage("XNET STARTUP",status);
        return XT_ERR_IO;
    }
    const network_ready::Result ready=xbox_network_ready::wait();
    g_netdiag.link_flags=ready.sample.link_flags;
    g_netdiag.xnaddr_flags=ready.sample.address_flags;
    g_netdiag.local_ip=ready.sample.local_ip;
    g_netdiag.address_state=(int)ready.state;
    g_netdiag.address_wait_ms=ready.waited_ms;
    char trace[160];
    sprintf_s(trace,sizeof(trace),
        "[XboxTLS] SYSAPP %s wait=%u link=%08X xnaddr=%08X\n",
        network_ready::text(ready.state),ready.waited_ms,
        ready.sample.link_flags,ready.sample.address_flags);
    OutputDebugStringA(trace);
    net_stage("WSA STARTUP",0);
    WSADATA wd;
    const int err=WSAStartup(MAKEWORD(2, 2), &wd);
    if (err != 0) {
        net_stage("WSA STARTUP",err);
        NetDll_XNetCleanup(XNCALLER_SYSAPP);
        return XT_ERR_IO;
    }
    net_stage("READY",0);
    return XT_OK;
}
void xbox360_network_cleanup() {
    WSACleanup();
    NetDll_XNetCleanup(XNCALLER_SYSAPP);
}

Xbox360SocketStream::Xbox360SocketStream() : socket_(INVALID_SOCKET) {}
Xbox360SocketStream::~Xbox360SocketStream() {
    close();
}
bool Xbox360SocketStream::is_open() const {
    return socket_ != INVALID_SOCKET;
}

Error Xbox360SocketStream::connect_tcp(const char* hostname, xt_u16 port) {
    if(!hostname || !*hostname || !port)return XT_ERR_INVALID_ARGUMENT;
    close();
    IN_ADDR targets[4];
    unsigned int target_count=0;
    net_stage("DNS XNET LOOKUP",0);
    XNDNS* dns=0;
    const int dns_result=NetDll_XNetDnsLookup(XNCALLER_SYSAPP,hostname,0,&dns);
    if(dns_result==0 && dns) {
        const DWORD begin=GetTickCount();
        while(dns->iStatus==WSAEINPROGRESS &&
              (DWORD)(GetTickCount()-begin)<6000u)Sleep(20);
        if(dns->iStatus==0 && dns->cina>0) {
            for(unsigned int i=0;i<dns->cina && target_count<4;++i) {
                const IN_ADDR candidate=dns->aina[i];
                bool duplicate=false;
                for(unsigned int j=0;j<target_count;++j)
                    if(targets[j].s_addr==candidate.s_addr)duplicate=true;
                if(!duplicate && candidate.s_addr && candidate.s_addr!=0xffffffffu)
                    targets[target_count++]=candidate;
            }
        } else net_stage("DNS XNET RESULT",
            dns->iStatus==WSAEINPROGRESS?WSAETIMEDOUT:
            (dns->iStatus?dns->iStatus:WSAHOST_NOT_FOUND));
    } else net_stage("DNS XNET CALL",dns_result?dns_result:WSAGetLastError());
    if(dns)NetDll_XNetDnsRelease(XNCALLER_SYSAPP,dns);
    if(!target_count) {
        net_stage("DNS PUBLIC QUERY",0);
        const UdpDnsResult fallback=udp_dns_lookup_google(hostname);
        if(fallback.state!=1 || fallback.address_count<=0) {
            net_stage(fallback.transport==2?"DNS PUBLIC TCP FAILED":"DNS PUBLIC UDP FAILED",
                      (int)fallback.error);
            return XT_ERR_IO;
        }
        for(int i=0;i<fallback.address_count && target_count<4;++i) {
            const IN_ADDR candidate=fallback.addresses[i];
            bool duplicate=false;
            for(unsigned int j=0;j<target_count;++j)
                if(targets[j].s_addr==candidate.s_addr)duplicate=true;
            if(!duplicate && candidate.s_addr && candidate.s_addr!=0xffffffffu)
                targets[target_count++]=candidate;
        }
    }
    g_netdiag.addresses=target_count;
    if(!target_count) {net_stage("DNS NO USABLE IPV4",WSAHOST_NOT_FOUND);return XT_ERR_IO;}
    const DWORD total_begin=GetTickCount();
    int last_error=WSAETIMEDOUT;
    for(unsigned int index=0;index<target_count;++index) {
        const DWORD elapsed=GetTickCount()-total_begin;
        if(elapsed>=12000u)break;
        // Share one bounded budget across all returned IPv4 addresses.
        const DWORD budget=(12000u-elapsed)/(target_count-index);
        g_netdiag.connect_attempts=index+1;
        g_netdiag.peer_ip=targets[index].s_addr;
        net_stage("SOCKET CREATE",0);
        socket_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(socket_==INVALID_SOCKET) {
            net_stage("SOCKET CREATE",WSAGetLastError());return XT_ERR_IO;
        }
        const BOOL public_socket=TRUE;
        if(setsockopt(socket_,SOL_SOCKET,SO_MARKINSECURE,
                      (const char*)&public_socket,sizeof(public_socket))==SOCKET_ERROR) {
            net_stage("SOCKET PUBLIC POLICY",WSAGetLastError());close();return XT_ERR_IO;
        }
        u_long nonblocking=1;
        if(ioctlsocket(socket_,FIONBIO,&nonblocking)==SOCKET_ERROR) {
            net_stage("SOCKET MODE",WSAGetLastError());close();return XT_ERR_IO;
        }
        sockaddr_in endpoint;
        memset(&endpoint,0,sizeof(endpoint));
        endpoint.sin_family=AF_INET;
        endpoint.sin_port=htons(port);
        endpoint.sin_addr=targets[index];
        const DWORD attempt_begin=GetTickCount();
        bool writable=false;
        net_stage("TCP CONNECT",0);
        const int rc=::connect(socket_,(sockaddr*)&endpoint,sizeof(endpoint));
        if(rc==0)writable=true;
        else {
            last_error=WSAGetLastError();
            if(last_error==WSAEWOULDBLOCK || last_error==WSAEINPROGRESS ||
               last_error==WSAEALREADY) {
                fd_set writes,failed;
                FD_ZERO(&writes);FD_ZERO(&failed);
                FD_SET(socket_,&writes);FD_SET(socket_,&failed);
                timeval deadline;
                deadline.tv_sec=(long)(budget/1000u);
                deadline.tv_usec=(long)((budget%1000u)*1000u);
                net_stage("TCP SELECT",0);
                const int selected=select(0,NULL,&writes,&failed,&deadline);
                writable=selected>0 && !FD_ISSET(socket_,&failed) && FD_ISSET(socket_,&writes);
                if(!writable) {
                    last_error=selected==0?WSAETIMEDOUT:
                        (selected<0?WSAGetLastError():WSAECONNREFUSED);
                    net_stage("TCP SELECT",last_error);
                }
            } else net_stage("TCP CONNECT",last_error);
        }
        g_netdiag.connect_ms=(unsigned int)(GetTickCount()-total_begin);
        const unsigned char* ip=(const unsigned char*)&targets[index].s_addr;
        char trace[200];
        sprintf_s(trace,sizeof(trace),
            "[XboxTLS] %.80s %u.%u.%u.%u:%u attempt=%u/%u ms=%lu writable=%d error=%d\n",
            hostname,ip[0],ip[1],ip[2],ip[3],(unsigned)port,index+1,target_count,
            (unsigned long)(GetTickCount()-attempt_begin),writable?1:0,writable?0:last_error);
        OutputDebugStringA(trace);
        if(!writable) {close();continue;}
        nonblocking=0;
        if(ioctlsocket(socket_,FIONBIO,&nonblocking)==SOCKET_ERROR) {
            net_stage("SOCKET BLOCKING",WSAGetLastError());close();return XT_ERR_IO;
        }
        // Writable is provisional. The original hostname, SNI and certificate
        // validation are still required in the TLS handshake above this layer.
        net_stage("TCP CONNECTED",0);
        return XT_OK;
    }
    g_netdiag.connect_ms=(unsigned int)(GetTickCount()-total_begin);
    net_stage("TCP ALL ADDRESSES FAILED",last_error);
    close();
    return XT_ERR_IO;
}

Error Xbox360SocketStream::set_io_timeouts(xt_u32 send_timeout_ms, xt_u32 receive_timeout_ms) {
    if (socket_ == INVALID_SOCKET)
        return XT_ERR_CLOSED;

    DWORD send_timeout = (DWORD)send_timeout_ms;
    DWORD receive_timeout = (DWORD)receive_timeout_ms;

    if (setsockopt(
            socket_,
            SOL_SOCKET,
            SO_SNDTIMEO,
            reinterpret_cast<const char*>(&send_timeout),
            sizeof(send_timeout)) == SOCKET_ERROR) {
        net_stage("SOCKET SEND TIMEOUT",WSAGetLastError());
        return XT_ERR_IO;
    }

    if (setsockopt(
            socket_,
            SOL_SOCKET,
            SO_RCVTIMEO,
            reinterpret_cast<const char*>(&receive_timeout),
            sizeof(receive_timeout)) == SOCKET_ERROR) {
        net_stage("SOCKET RECEIVE TIMEOUT",WSAGetLastError());
        return XT_ERR_IO;
    }

    return XT_OK;
}

Error Xbox360SocketStream::send_all(ByteSpan data) {
    if (socket_ == INVALID_SOCKET || (!data.data && data.size))
        return XT_ERR_INVALID_ARGUMENT;
    size_t off = 0;
    while (off < data.size) {
        int chunk = (data.size - off > 0x7fffffffU) ? 0x7fffffff : (int)(data.size - off);
        int n = ::send(socket_, (const char*)data.data + off, chunk, 0);
        if (n <= 0) {
            net_stage("TCP SEND",WSAGetLastError());
            return XT_ERR_IO;
        }
        off += (size_t)n;
    }
    return XT_OK;
}
Error Xbox360SocketStream::recv_some(MutableByteSpan buffer, size_t* received) {
    if (!received || socket_ == INVALID_SOCKET || (!buffer.data && buffer.size))
        return XT_ERR_INVALID_ARGUMENT;
    *received = 0;
    if (!buffer.size)
        return XT_OK;
    int cap = buffer.size > 0x7fffffffU ? 0x7fffffff : (int)buffer.size;
    int n = ::recv(socket_, (char*)buffer.data, cap, 0);
    if (n == 0)
        return XT_ERR_CLOSED;
    if (n < 0) {
        net_stage("TCP RECEIVE",WSAGetLastError());
        return XT_ERR_IO;
    }
    *received = (size_t)n;
    return XT_OK;
}
void Xbox360SocketStream::close() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

} // namespace xboxtls
#endif
