#include "xboxtls/xbox360_platform.h"

#if defined(_XBOX)
#include <string.h>
#include <winsockx.h>
#include "xkelib.h"
#include "syssock.h"
#include "udp_dns.h" // Reuse the Xbox 360 public DNS fallbacks.

namespace xboxtls {

// Human-readable network diagnostics for RGH hardware and Xenia.
// All stored values are non-sensitive. No authentication data.
static XboxNetworkDiagnostic g_netdiag = {"NOT STARTED",0,0,0,0,0};
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
    net_stage("XNET STARTUP",0);
    XNetStartupParams xn;
    memset(&xn, 0, sizeof(xn));
    xn.cfgSizeOfStruct = sizeof(xn);
    xn.cfgFlags = 0; // RETAIL safe; no devkit-only security bypass.
    const int status=XNetStartup(&xn);
    if (status != 0) {
        net_stage("XNET STARTUP",status);
        return XT_ERR_IO;
    }
    g_netdiag.link_flags=XNetGetEthernetLinkStatus();
    XNADDR address;
    memset(&address,0,sizeof(address));
    g_netdiag.xnaddr_flags=XNetGetTitleXnAddr(&address);
    g_netdiag.local_ip=address.ina.s_addr;
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
    if (!hostname || !*hostname || port==0)
        return XT_ERR_INVALID_ARGUMENT;
    close();
    net_stage("DNS XNET LOOKUP",0);
    IN_ADDR resolved;
    memset(&resolved,0,sizeof(resolved));
    bool have_address=false;
    XNDNS* dns=0;
    const int dns_result=NetDll_XNetDnsLookup(XNCALLER_SYSAPP,hostname,0,&dns);
    if(dns_result==0 && dns) {
        const DWORD start=GetTickCount();
        while(dns->iStatus==WSAEINPROGRESS &&
              (DWORD)(GetTickCount()-start)<6000u)
            Sleep(20);
        if(dns->iStatus==0 && dns->cina>0) {
            resolved=dns->aina[0];
            g_netdiag.addresses=(unsigned int)dns->cina;
            have_address=true;
        } else {
            net_stage("DNS XNET RESULT",
                dns->iStatus==WSAEINPROGRESS ? WSAETIMEDOUT :
                (dns->iStatus!=0 ? dns->iStatus : WSAHOST_NOT_FOUND));
        }
    } else {
        net_stage("DNS XNET CALL",
            dns_result!=0 ? dns_result : WSAGetLastError());
    }
    if(dns)NetDll_XNetDnsRelease(XNCALLER_SYSAPP,dns);

    if(!have_address) {
        // Public DNS fallback can help Xenia if its XNet resolver returns
        // no records. UDP/53 then TCP/53 are real DNS; never a fake address.
        // TLS still validates the ORIGINAL hostname via SNI and X.509.
        net_stage("DNS PUBLIC QUERY",0);
        const UdpDnsResult external=udp_dns_lookup_google(hostname);
        if(external.state!=1 || external.address_count<=0) {
            net_stage(external.transport==2 ?
                      "DNS PUBLIC TCP FAILED" : "DNS PUBLIC UDP FAILED",
                      (int)external.error);
            return XT_ERR_IO;
        }
        resolved=external.addresses[0];
        g_netdiag.addresses=(unsigned int)external.address_count;
        net_stage(external.transport==2 ?
                  "DNS PUBLIC TCP OK" : "DNS PUBLIC UDP OK",0);
    }

    net_stage("SOCKET CREATE",0);
    socket_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(socket_==INVALID_SOCKET) {
        net_stage("SOCKET CREATE",WSAGetLastError());
        return XT_ERR_IO;
    }
    u_long nonblocking=1;
    if(ioctlsocket(socket_,FIONBIO,&nonblocking)==SOCKET_ERROR) {
        net_stage("SOCKET MODE",WSAGetLastError());
        close();
        return XT_ERR_IO;
    }
    sockaddr_in endpoint;
    memset(&endpoint,0,sizeof(endpoint));
    endpoint.sin_family=AF_INET;
    endpoint.sin_port=htons(port);
    endpoint.sin_addr=resolved;
    net_stage("TCP CONNECT",0);
    const int rc=::connect(socket_,(sockaddr*)&endpoint,sizeof(endpoint));
    if(rc==SOCKET_ERROR) {
        const int err=WSAGetLastError();
        if(err!=WSAEWOULDBLOCK && err!=WSAEINPROGRESS &&
           err!=WSAEALREADY) {
            net_stage("TCP CONNECT",err);
            close();
            return XT_ERR_IO;
        }
        fd_set writable,failed;
        FD_ZERO(&writable);
        FD_ZERO(&failed);
        FD_SET(socket_,&writable);
        FD_SET(socket_,&failed);
        timeval deadline;
        deadline.tv_sec=8;
        deadline.tv_usec=0;
        net_stage("TCP SELECT",0);
        const int selected=select(0,NULL,&writable,&failed,&deadline);
        if(selected<=0 || !FD_ISSET(socket_,&writable)) {
            net_stage("TCP SELECT",selected==0 ? WSAETIMEDOUT :
                      (selected<0 ? WSAGetLastError():WSAECONNREFUSED));
            close();
            return XT_ERR_IO;
        }
        // The Xbox SDK lacks SO_ERROR and Xenia does not implement
        // NetDll_getpeername. Writability is only a provisional signal.
        // The caller MUST send the TLS ClientHello and validate the
        // authenticated TLS server response before considering HTTPS OK.
    }
    nonblocking=0;
    if(ioctlsocket(socket_,FIONBIO,&nonblocking)==SOCKET_ERROR) {
        net_stage("SOCKET BLOCKING",WSAGetLastError());
        close();
        return XT_ERR_IO;
    }
    net_stage("TCP CONNECTED",0);
    return XT_OK;
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
