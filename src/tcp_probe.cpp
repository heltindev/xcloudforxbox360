#include "tcp_probe.h"
#include "platform.h"
#include "udp_dns.h"
#include <xtl.h>
#include <winsockx.h>
#include <stdio.h>

// Two independent transport checks: hostname-based Xbox endpoint and
// direct public IPv4 address (1.1.1.1), both port 443. No TLS or HTTP.
namespace {
struct TestOutcome { DWORD error; int source; };

TestOutcome attempt_tcp(const IN_ADDR& ip, DWORD deadline_ms) {
    TestOutcome result = { 0, TcpProbe::NONE };
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        result.error = (DWORD)WSAGetLastError();
        result.source = TcpProbe::SOCKET_CREATE;
        return result;
    }

    u_long nonblocking = 1;
    if (ioctlsocket(sock, FIONBIO, &nonblocking) == SOCKET_ERROR) {
        result.error = (DWORD)WSAGetLastError();
        result.source = TcpProbe::SOCKET_MODE;
        closesocket(sock);
        return result;
    }

    sockaddr_in endpoint;
    ZeroMemory(&endpoint, sizeof(endpoint));
    endpoint.sin_family = AF_INET;
    endpoint.sin_port = htons(443);
    endpoint.sin_addr = ip;

    const int attempt = connect(sock, (const sockaddr*)&endpoint, sizeof(endpoint));
    if (attempt == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS && err != WSAEALREADY) {
            result.error = (DWORD)err;
            result.source = TcpProbe::TCP_CONNECT;
            closesocket(sock);
            return result;
        }

        fd_set writeable, exceptions;
        FD_ZERO(&writeable);
        FD_ZERO(&exceptions);
        FD_SET(sock, &writeable);
        FD_SET(sock, &exceptions);
        timeval timeout;
        timeout.tv_sec = (long)(deadline_ms / 1000);
        timeout.tv_usec = (long)((deadline_ms % 1000) * 1000);
        const int selected = select(0, NULL, &writeable, &exceptions, &timeout);
        if (selected <= 0) {
            result.error = selected == 0 ? (DWORD)WSAETIMEDOUT : (DWORD)WSAGetLastError();
            result.source = TcpProbe::TCP_SELECT;
            closesocket(sock);
            return result;
        }
        // The Xbox 360 SDK does not expose SO_ERROR. Verify that the TCP
        // connection is actually established, not merely signaled writable.
        if (!FD_ISSET(sock, &writeable)) {
            result.error = (DWORD)WSAECONNREFUSED;
            result.source = TcpProbe::TCP_RESULT;
            closesocket(sock);
            return result;
        }
        sockaddr_in peer;
        int peer_len = sizeof(peer);
        ZeroMemory(&peer, sizeof(peer));
        if (getpeername(sock, (sockaddr*)&peer, &peer_len) == SOCKET_ERROR) {
            result.error = (DWORD)WSAGetLastError();
            result.source = TcpProbe::TCP_RESULT;
            closesocket(sock);
            return result;
        }
    }
    closesocket(sock);
    return result;
}
}

TcpProbe::TcpProbe()
    : thread_(NULL), state_(IDLE), stage_(0), error_(0), source_(NONE),
      xbox_result_(0), control_result_(0), xbox_error_(0), control_error_(0),
      dns_addresses_(0), dns_state_(0), dns_error_(0),
      xbox_error_source_(NONE), control_error_source_(NONE),
      udp_dns_state_(0), udp_dns_error_(0), udp_dns_error_source_(0),
      udp_dns_addresses_(0), dns_path_(0) {}

TcpProbe::~TcpProbe() {
    if (thread_) {
        WaitForSingleObject(thread_, INFINITE);
        CloseHandle(thread_);
        thread_ = NULL;
    }
}
bool TcpProbe::start() {
    update();
    if (state_ == RUNNING || thread_) return false;
    stage_ = 1;
    error_ = 0;
    source_ = NONE;
    xbox_result_ = 0;
    control_result_ = 0;
    xbox_error_ = 0;
    control_error_ = 0;
    dns_addresses_ = 0;
    dns_state_ = 0;
    dns_error_ = 0;
    xbox_error_source_ = NONE;
    control_error_source_ = NONE;
    udp_dns_state_ = 0;
    udp_dns_error_ = 0;
    udp_dns_error_source_ = 0;
    udp_dns_addresses_ = 0;
    dns_path_ = 0;
    state_ = RUNNING;
    thread_ = CreateThread(NULL, 0, worker, this, 0, NULL);
    if (!thread_) {
        error_ = (LONG)GetLastError();
        source_ = THREAD_CREATE;
        state_ = FAILED;
        return false;
    }
    return true;
}
void TcpProbe::update() {
    if (thread_ && WaitForSingleObject(thread_, 0) == WAIT_OBJECT_0) {
        CloseHandle(thread_);
        thread_ = NULL;
    }
}
TcpProbe::State TcpProbe::state() const { return (State)state_; }
int TcpProbe::stage() const { return (int)stage_; }
unsigned long TcpProbe::error() const { return (unsigned long)error_; }
int TcpProbe::error_source() const { return (int)source_; }
int TcpProbe::xbox_result() const { return (int)xbox_result_; }
int TcpProbe::control_result() const { return (int)control_result_; }
unsigned long TcpProbe::xbox_error() const { return (unsigned long)xbox_error_; }
unsigned long TcpProbe::control_error() const { return (unsigned long)control_error_; }
int TcpProbe::dns_addresses() const { return (int)dns_addresses_; }
int TcpProbe::dns_state() const { return (int)dns_state_; }
unsigned long TcpProbe::dns_error() const { return (unsigned long)dns_error_; }
int TcpProbe::xbox_error_source() const { return (int)xbox_error_source_; }
int TcpProbe::control_error_source() const { return (int)control_error_source_; }
int TcpProbe::udp_dns_state() const { return (int)udp_dns_state_; }
unsigned long TcpProbe::udp_dns_error() const { return (unsigned long)udp_dns_error_; }
int TcpProbe::udp_dns_error_source() const { return (int)udp_dns_error_source_; }
int TcpProbe::udp_dns_addresses() const { return (int)udp_dns_addresses_; }
int TcpProbe::dns_path() const { return (int)dns_path_; }

DWORD WINAPI TcpProbe::worker(LPVOID context) {
    static_cast<TcpProbe*>(context)->perform();
    return 0;
}
void TcpProbe::perform() {
    bool xnet = false, winsock = false;
    XNDNS* dns = NULL;
    IN_ADDR targets[3];
    int target_count = 0;
    WSADATA wsa;
    ZeroMemory(&wsa, sizeof(wsa));
    char log[180];

    stage_ = 1;
    XNetStartupParams params;
    ZeroMemory(&params, sizeof(params));
    params.cfgSizeOfStruct = sizeof(params);
    // Devkit-only override; retail XNet generally ignores this flag.
    params.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    {
        const int status = XNetStartup(&params);
        if (status != 0) { error_ = status; source_ = XNET_INIT; goto cleanup; }
    }
    xnet = true;

    stage_ = 2;
    {
        const int status = WSAStartup(MAKEWORD(2,2), &wsa);
        if (status != 0) { error_ = status; source_ = WSA_INIT; goto cleanup; }
    }
    winsock = true;

    stage_ = 3;
    {
        const int status = XNetDnsLookup("www.xbox.com", NULL, &dns);
        if (status != 0 || !dns) {
            error_ = status != 0 ? status : (LONG)WSAGetLastError();
            dns_state_ = 2;
            dns_error_ = (LONG)error_;
            source_ = DNS_CALL;
        } else {
            const DWORD tick = GetTickCount();
            while (dns->iStatus == WSAEINPROGRESS &&
                   (DWORD)(GetTickCount() - tick) < 8000) Sleep(50);
            if (dns->iStatus == 0 && dns->cina != 0) {
                dns_state_ = 1;
                dns_path_ = 1;
                dns_addresses_ = (LONG)dns->cina;
                target_count = (dns->cina < 3) ? (int)dns->cina : 3;
                for (int i = 0; i < target_count; ++i) targets[i] = dns->aina[i];
            } else {
                // No A records is not a WinSock error code; report it separately.
                dns_error_ = dns->iStatus != 0 ? dns->iStatus : 0;
                dns_state_ = 2;
                error_ = (LONG)dns_error_;
                source_ = DNS_STATUS;
            }
        }
    }
    if (dns) { XNetDnsRelease(dns); dns = NULL; }

    // Stage 4: DNS over UDP instead of relying only on XNetDnsLookup.
    // Google DNS is a control endpoint and does not change console settings.
    stage_ = 4;
    if (target_count == 0) {
        // UDP fallback is only needed if the Xbox resolver failed.
        // Preserve the dashboard DNS configuration as the primary path.
        const UdpDnsResult external = udp_dns_lookup_google("www.xbox.com");
        udp_dns_state_ = external.state;
        udp_dns_error_ = (LONG)external.error;
        udp_dns_error_source_ = external.error_source;
        udp_dns_addresses_ = external.address_count;
        if (target_count == 0 && external.state == 1) {
            target_count = external.address_count;
            if (target_count > 3) target_count = 3;
            for (int i = 0; i < target_count; ++i)
                targets[i] = external.addresses[i];
            dns_path_ = 2;
        }
    }

    stage_ = 5;
    if (target_count == 0) {
        xbox_result_ = 2;
        xbox_error_ = udp_dns_error_ != 0 ? udp_dns_error_ : dns_error_;
        xbox_error_source_ = udp_dns_error_ != 0
            ? (LONG)(10 + udp_dns_error_source_)
            : (LONG)source_;
    } else {
        for (int i = 0; i < target_count; ++i) {
            TestOutcome o = attempt_tcp(targets[i], 6000);
            xbox_error_ = (LONG)o.error;
            xbox_error_source_ = (LONG)o.source;
            if (o.error == 0) {
                xbox_result_ = 1;
                xbox_error_ = 0;
                break;
            }
            xbox_result_ = 2;
            source_ = (LONG)o.source;
            error_ = (LONG)o.error;
        }
    }

    // Numeric IP control bypasses DNS and tests whether *any* public
    // TCP target works. A successful control does not imply xCloud works.
    stage_ = 6;
    {
        IN_ADDR control;
        control.s_addr = inet_addr("1.1.1.1");
        const TestOutcome o = attempt_tcp(control, 6000);
        control_error_ = (LONG)o.error;
        control_error_source_ = (LONG)o.source;
        control_result_ = o.error == 0 ? 1 : 2;
        if (o.error != 0) {
            error_ = (LONG)o.error;
            source_ = (LONG)o.source;
        }
    }

    stage_ = 7;
    if (xbox_result_ == 1 || control_result_ == 1) {
        error_ = 0;
        source_ = NONE;
        state_ = CONNECTED;
    } else {
        state_ = FAILED;
    }

cleanup:
    if (dns) XNetDnsRelease(dns);
    if (winsock) WSACleanup();
    if (xnet) XNetCleanup();
    if (state_ == RUNNING) state_ = FAILED;
    sprintf_s(log,sizeof(log),
              "[XCloud360] probe state=%ld xbox=%ld(%ld) ctrl=%ld(%ld) xnetdns=%ld udpdns=%ld(%ld) path=%ld src=%ld err=%ld\n",
              state_, xbox_result_, xbox_error_, control_result_, control_error_,
              dns_addresses_, udp_dns_state_, udp_dns_error_, dns_path_, source_, error_);
    platform::debug_log(log);
}
