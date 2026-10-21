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
    if (result.state == 1) result.transport = 1;
    return result;
}
bool send_exact(SOCKET sock, const unsigned char* p, int size) {
    int total=0;
    while(total<size) {
        const int n=send(sock,(const char*)p+total,size-total,0);
        if(n<=0)return false;
        total+=n;
    }
    return true;
}
bool receive_exact(SOCKET sock, unsigned char* p, int size) {
    int total=0;
    while(total<size) {
        const int n=recv(sock,(char*)p+total,size-total,0);
        if(n<=0)return false;
        total+=n;
    }
    return true;
}
// RFC 1035 DNS over TCP fallback. The DNS packet is prefixed by a 16-bit
// big-endian length. Uses only numeric IPs; does not rely on Xbox XNet DNS.
// DNS by itself is not trusted for identity: the TLS client MUST verify
// the original hostname against the HTTPS certificate after TCP connects.
UdpDnsResult try_tcp_server(const char* hostname,const char* server) {
    UdpDnsResult result;
    memset(&result,0,sizeof(result));
    result.state=2;
    result.transport=2;
    const unsigned long server_ip=inet_addr(server);
    result.server=server_ip;
    unsigned char question[kMaxPacket+2];
    const unsigned int tid=(unsigned int)(GetTickCount()&0xffffu);
    const int length=build_question(question+2,kMaxPacket,hostname,tid);
    if(length<=0) {result.error=0xE101;result.error_source=10;return result;}
    question[0]=(unsigned char)(length>>8);
    question[1]=(unsigned char)length;
    SOCKET sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sock==INVALID_SOCKET) {
        result.error=(unsigned long)WSAGetLastError();
        result.error_source=6;
        return result;
    }
    u_long enabled=1;
    if(ioctlsocket(sock,FIONBIO,&enabled)==SOCKET_ERROR) {
        result.error=(unsigned long)WSAGetLastError();
        result.error_source=7;
        closesocket(sock);
        return result;
    }
    sockaddr_in endpoint;
    memset(&endpoint,0,sizeof(endpoint));
    endpoint.sin_family=AF_INET;
    endpoint.sin_port=htons(53);
    endpoint.sin_addr.s_addr=server_ip;
    const int connected=connect(sock,(const sockaddr*)&endpoint,sizeof(endpoint));
    if(connected==SOCKET_ERROR) {
        const int err=WSAGetLastError();
        if(err!=WSAEWOULDBLOCK && err!=WSAEINPROGRESS &&
           err!=WSAEALREADY) {
            result.error=(unsigned long)err;
            result.error_source=7;
            closesocket(sock);
            return result;
        }
        fd_set writable,failed;
        FD_ZERO(&writable);FD_ZERO(&failed);
        FD_SET(sock,&writable);FD_SET(sock,&failed);
        timeval wait;
        wait.tv_sec=3;
        wait.tv_usec=0;
        const int n=select(0,NULL,&writable,&failed,&wait);
        if(n<=0 || !FD_ISSET(sock,&writable)) {
            result.error=n==0 ? WSAETIMEDOUT :
                         (n<0 ? (unsigned long)WSAGetLastError() :
                                (unsigned long)WSAECONNREFUSED);
            result.error_source=7;
            closesocket(sock);
            return result;
        }
        // Xenia does not implement NetDll_getpeername. Do NOT treat
        // select(writable) as success: an actual framed DNS request is
        // sent and a matching DNS reply is required further below.
    }
    enabled=0;
    if(ioctlsocket(sock,FIONBIO,&enabled)==SOCKET_ERROR) {
        result.error=(unsigned long)WSAGetLastError();
        result.error_source=7;
        closesocket(sock);
        return result;
    }
    DWORD timeout=4000;
    if(setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,
                  sizeof(timeout))==SOCKET_ERROR ||
       setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,
                  sizeof(timeout))==SOCKET_ERROR) {
        result.error=(unsigned long)WSAGetLastError();
        result.error_source=8;
        closesocket(sock);
        return result;
    }
    if(!send_exact(sock,question,length+2)) {
        result.error=(unsigned long)WSAGetLastError();
        result.error_source=8;
        closesocket(sock);
        return result;
    }
    unsigned char header[2];
    if(!receive_exact(sock,header,2)) {
        result.error=(unsigned long)WSAGetLastError();
        result.error_source=9;
        closesocket(sock);
        return result;
    }
    const unsigned int reply_length=get16(header);
    if(reply_length<12 || reply_length>kMaxPacket) {
        result.error=0xE102;
        result.error_source=10;
        closesocket(sock);
        return result;
    }
    unsigned char reply[kMaxPacket];
    if(!receive_exact(sock,reply,(int)reply_length)) {
        result.error=(unsigned long)WSAGetLastError();
        result.error_source=9;
        closesocket(sock);
        return result;
    }
    closesocket(sock);
    if(!parse_reply(reply,(int)reply_length,tid,&result)) {
        result.error=0xE103;
        result.error_source=10;
        return result;
    }
    result.state=1;
    result.error=0;
    result.error_source=0;
    return result;
}
}

UdpDnsResult udp_dns_lookup_google(const char* hostname) {
    // Xbox resolver is always tried FIRST in each caller. On failure,
    // probe public UDP DNS, then DNS over TCP (Cloudflare/Google).
    UdpDnsResult google_udp=try_server(hostname,"8.8.8.8");
    if(google_udp.state==1)return google_udp;
    UdpDnsResult google_udp_alt=try_server(hostname,"8.8.4.4");
    if(google_udp_alt.state==1)return google_udp_alt;
    UdpDnsResult cloudflare_tcp=try_tcp_server(hostname,"1.1.1.1");
    if(cloudflare_tcp.state==1)return cloudflare_tcp;
    UdpDnsResult google_tcp=try_tcp_server(hostname,"8.8.8.8");
    if(google_tcp.state==1)return google_tcp;
    // Do not fake a successful lookup if the provider is blocked.
    return cloudflare_tcp;
}
