#pragma once
#include <xtl.h>

// Diagnostic only. No TLS, Microsoft login, tokens, or game streaming.
class TcpProbe {
public:
    enum State { IDLE=0, RUNNING=1, CONNECTED=2, FAILED=3 };
    enum ErrorSource {
        NONE=0, XNET_INIT=1, WSA_INIT=2, DNS_CALL=3,
        DNS_STATUS=4, SOCKET_CREATE=5, SOCKET_MODE=6,
        TCP_CONNECT=7, TCP_SELECT=8, TCP_RESULT=9, THREAD_CREATE=10,
        DNS_UDP_SOCKET=11, DNS_UDP_SEND=12, DNS_UDP_WAIT=13,
        DNS_UDP_RECEIVE=14, DNS_UDP_RESPONSE=15,
        DNS_TCP_SOCKET=16, DNS_TCP_CONNECT=17, DNS_TCP_SEND=18,
        DNS_TCP_RECEIVE=19, DNS_TCP_RESPONSE=20
    };
    TcpProbe();
    ~TcpProbe();
    bool start();
    void update();
    State state() const;
    int stage() const;
    unsigned long error() const;
    int error_source() const;
    int xbox_result() const;   // 0 pending, 1 protocol-verified, 2 failed, 3 TCP writable only
    int control_result() const;
    unsigned long xbox_error() const;
    unsigned long control_error() const;
    int dns_addresses() const;
    int dns_state() const;  // 0 waiting, 1 addresses resolved, 2 DNS failed
    unsigned long dns_error() const;
    int xbox_error_source() const;
    int control_error_source() const;
    int udp_dns_state() const;
    unsigned long udp_dns_error() const;
    int udp_dns_error_source() const;
    int udp_dns_addresses() const;
    int dns_path() const; // 0 none, 1 Xbox XNet, 2 public UDP, 3 public TCP
    int control80_result() const;
    unsigned long control80_error() const;
    int control80_error_source() const;
    unsigned long link_flags() const;
    unsigned long address_flags() const;
    unsigned long local_ip() const;
private:
    static DWORD WINAPI worker(LPVOID context);
    void perform();
    HANDLE thread_;
    volatile LONG state_;
    volatile LONG stage_;
    volatile LONG error_;
    volatile LONG source_;
    volatile LONG xbox_result_;
    volatile LONG control_result_;
    volatile LONG xbox_error_;
    volatile LONG control_error_;
    volatile LONG dns_addresses_;
    volatile LONG dns_state_;
    volatile LONG dns_error_;
    volatile LONG xbox_error_source_;
    volatile LONG control_error_source_;
    volatile LONG udp_dns_state_;
    volatile LONG udp_dns_error_;
    volatile LONG udp_dns_error_source_;
    volatile LONG udp_dns_addresses_;
    volatile LONG dns_path_;
    volatile LONG control80_result_;
    volatile LONG control80_error_;
    volatile LONG control80_error_source_;
    volatile LONG link_flags_;
    volatile LONG address_flags_;
    volatile LONG local_ip_;
};
