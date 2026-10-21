#pragma once
#include <xtl.h>
#include <winsockx.h>

// Diagnostic DNS over UDP, independent from Xbox XNetDnsLookup.
// No credentials, TLS or API tokens. This is an unauthenticated DNS test.
struct UdpDnsResult {
    int state; // 0 not attempted, 1 resolved, 2 failed
    unsigned long error;
    int error_source; // 1 socket, 2 send, 3 wait, 4 receive, 5 response
    int address_count;
    IN_ADDR addresses[3];
    unsigned long server; // IP address in network byte order
    int transport; // 0 unknown, 1 UDP/53, 2 TCP/53
};

UdpDnsResult udp_dns_lookup_google(const char* host);
