#include "udp_dns.h"
#include <xtl.h>
#include <winsockx.h>
#include <string.h>

// Google Public DNS is an OPTIONAL control test, not a change to the
// Xbox 360 dashboard network settings. Normal DNS uses XNetDnsLookup.
namespace {
const int kMaxPacket = 512;
const int kMaxAddresses = 3;
unsigned int get16(const unsigned char* p) {
    return ((unsigned int)p[0] << 8) | (unsigned int)p[1];
}
bool skip_name(const unsigned char* p, int length, int* at) {
    int i = *at;
    // This only advances through a wire-format DNS name; do not follow
    // compression pointers because the target offset is not needed here.
    for (int hops=0; hops < 128; ++hops) {
        if (i >= length) return false;
        const unsigned int n = p[i++];
        if (n == 0) { *at = i; return true; }
        if ((n & 0xC0) == 0xC0) {
            if (i >= length) return false;
            *at = i + 1;
            return true;
        }
        if ((n & 0xC0) != 0 || n > 63 || i + (int)n > length)
            return false;
        i += n;
    }
    return false;
}
bool parse_reply(const unsigned char* p, int size,
                 unsigned int transaction, UdpDnsResult* result) {
    if (size < 12 || get16(p) != transaction) return false;
    const unsigned int flags = get16(p+2);
    if ((flags & 0x8000) == 0 || (flags & 0x000f) != 0 ||
        (flags & 0x0200) != 0 || (flags & 0x7800) != 0)
        return false;
    const unsigned int questions = get16(p+4);
    const unsigned int answers = get16(p+6);
    if (!questions || questions > 5 || answers > 80) return false;
    int off = 12;
    for (unsigned int q=0; q<questions; ++q) {
        if (!skip_name(p,size,&off) || off + 4 > size) return false;
        off += 4;
    }
    for (unsigned int a=0; a<answers; ++a) {
        if (!skip_name(p,size,&off) || off + 10 > size) return false;
        const unsigned int type = get16(p+off);
        const unsigned int cls = get16(p+off+2);
        const unsigned int dataSize = get16(p+off+8);
        off += 10;
        if (off + (int)dataSize > size) return false;
        if (type == 1 && cls == 1 && dataSize == 4 &&
            result->address_count < kMaxAddresses) {
            IN_ADDR& entry = result->addresses[result->address_count++];
            memcpy(&entry.s_addr, p+off, 4);
        }
        off += (int)dataSize;
    }
    return result->address_count > 0;
}
int build_question(unsigned char* out, int capacity,
                   const char* hostname, unsigned int transaction) {
    if (!out || !hostname || capacity < 32) return 0;
    memset(out,0,capacity);
    out[0] = (unsigned char)(transaction >> 8);
    out[1] = (unsigned char)transaction;
    out[2] = 0x01; // RD
    out[5] = 0x01; // QDCOUNT=1
    int offset = 12;
    const char* s = hostname;
    while (*s) {
        const char* end = s;
        while (*end && *end!='.') ++end;
        const int n=(int)(end-s);
        if (n < 1 || n > 63 || offset+n+6 > capacity) return 0;
        out[offset++] = (unsigned char)n;
        for (int i=0;i<n;++i) out[offset++] = (unsigned char)s[i];
        if (!*end) break;
        s = end+1;
    }
    out[offset++] = 0;
    out[offset++] = 0; out[offset++] = 1; // QTYPE A
    out[offset++] = 0; out[offset++] = 1; // QCLASS IN
    return offset;
}
UdpDnsResult try_server(const char* hostname, const char* server) {
    UdpDnsResult result;
    memset(&result,0,sizeof(result));
    result.state = 2;
    const unsigned long serverIP = inet_addr(server);
    result.server = serverIP;
    SOCKET sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        result.error = (unsigned long)WSAGetLastError();
        result.error_source = 1;
        return result;
    }
    sockaddr_in target;
    memset(&target,0,sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(53);
    target.sin_addr.s_addr = serverIP;
    unsigned char buffer[kMaxPacket];
    const unsigned int transaction = ((unsigned int)GetTickCount() & 0xffffu);
    const int packetSize = build_question(buffer,kMaxPacket,hostname,transaction);
    if (!packetSize) {
        result.error = 0xE001;
        result.error_source = 5;
    } else if (sendto(sock,(const char*)buffer,packetSize,0,
                      (const sockaddr*)&target,sizeof(target)) == SOCKET_ERROR) {
        result.error = (unsigned long)WSAGetLastError();
        result.error_source = 2;
    } else {
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET(sock,&readable);
        timeval timeout;
        timeout.tv_sec=3;
        timeout.tv_usec=0;
        const int ready = select(0,&readable,NULL,NULL,&timeout);
        if (ready <= 0) {
            result.error = ready == 0 ? WSAETIMEDOUT :
                                                (unsigned long)WSAGetLastError();
            result.error_source = 3;
        } else {
            sockaddr_in sender;
            int senderLen = sizeof(sender);
            memset(&sender,0,sizeof(sender));
            const int n = recvfrom(sock,(char*)buffer,sizeof(buffer),0,
                                   (sockaddr*)&sender,&senderLen);
            if (n == SOCKET_ERROR) {
                result.error = (unsigned long)WSAGetLastError();
                result.error_source = 4;
            } else if (sender.sin_addr.s_addr == serverIP &&
                       sender.sin_port == htons(53) &&
                       parse_reply(buffer,n,transaction,&result)) {
                result.state=1;
                result.error=0;
                result.error_source=0;
            } else {
                result.error=0xE002; // malformed, truncated, no A, or wrong reply
                result.error_source=5;
            }
        }
    }
    closesocket(sock);
    return result;
}
}

UdpDnsResult udp_dns_lookup_google(const char* hostname) {
    UdpDnsResult primary = try_server(hostname,"8.8.8.8");
    if (primary.state == 1) return primary;
    UdpDnsResult secondary = try_server(hostname,"8.8.4.4");
    return secondary.state == 1 ? secondary : primary;
}
