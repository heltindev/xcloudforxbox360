#ifndef XBOXTLS_XBOX360_PLATFORM_H
#define XBOXTLS_XBOX360_PLATFORM_H

#include "platform.h"

#if defined(_XBOX)
#include <xtl.h>

namespace xboxtls {

class Xbox360Platform : public Platform {
  public:
    virtual Error random_bytes(MutableByteSpan out);
    virtual xt_u64 unix_time_seconds();
};

class Xbox360SocketStream : public Stream {
  public:
    Xbox360SocketStream();
    virtual ~Xbox360SocketStream();
    Error connect_tcp(const char* hostname, xt_u16 port);
    Error set_io_timeouts(xt_u32 send_timeout_ms, xt_u32 receive_timeout_ms);
    virtual Error send_all(ByteSpan data);
    virtual Error recv_some(MutableByteSpan buffer, size_t* received);
    virtual void close();
    bool is_open() const;

  private:
    SOCKET socket_;
};

// Diagnostic snapshot for the network layer. Never contains credentials.
struct XboxNetworkDiagnostic {
    const char* stage;  // static literal: XNET/DNS/SOCKET/CONNECT/TLS
    int error;          // WinSock code, e.g. 10060 = WSAETIMEDOUT
    unsigned int addresses;
    unsigned long link_flags;
    unsigned long xnaddr_flags;
    unsigned long local_ip;
    int address_state;
    unsigned int address_wait_ms;
    unsigned int connect_attempts;
    unsigned long peer_ip;
    unsigned int connect_ms;
};
void xbox360_network_diagnostic(XboxNetworkDiagnostic* result);
Error xbox360_network_startup();
void xbox360_network_cleanup();

} // namespace xboxtls
#endif

#endif
