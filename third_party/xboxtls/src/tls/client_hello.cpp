#include "xboxtls/endian.h"
#include "xboxtls/tls13.h"
#include <string.h>

namespace xboxtls {

ClientHelloParams::ClientHelloParams() : group(GROUP_X25519), server_name(0), alpn(0) {}

class Writer {
  public:
    Writer(MutableByteSpan s) : p_(s.data), cap_(s.size), n_(0), ok_(s.data != 0) {}
    bool put8(xt_u8 v) {
        if (n_ + 1 > cap_)
            return ok_ = false;
        p_[n_++] = v;
        return true;
    }
    bool put16(xt_u16 v) {
        if (n_ + 2 > cap_)
            return ok_ = false;
        store_be16(p_ + n_, v);
        n_ += 2;
        return true;
    }
    bool put24(xt_u32 v) {
        if (n_ + 3 > cap_)
            return ok_ = false;
        store_be24(p_ + n_, v);
        n_ += 3;
        return true;
    }
    bool bytes(const void* p, size_t z) {
        if (n_ + z > cap_)
            return ok_ = false;
        if (z)
            memcpy(p_ + n_, p, z);
        n_ += z;
        return true;
    }
    bool patch16(size_t at, xt_u16 v) {
        if (at + 2 > n_)
            return false;
        store_be16(p_ + at, v);
        return true;
    }
    bool patch24(size_t at, xt_u32 v) {
        if (at + 3 > n_)
            return false;
        store_be24(p_ + at, v);
        return true;
    }
    size_t size() const {
        return n_;
    }
    bool ok() const {
        return ok_;
    }

  private:
    xt_u8* p_;
    size_t cap_, n_;
    bool ok_;
};

static bool ext_supported_versions(Writer& w) {
    w.put16(0x002b);
    w.put16(3);
    w.put8(2);
    w.put16(0x0304);
    return w.ok();
}
static bool ext_supported_groups(Writer& w, NamedGroup preferred) {
    w.put16(0x000a);
    w.put16(6);
    w.put16(4);
    w.put16((xt_u16)preferred);
    w.put16((xt_u16)(preferred == GROUP_X25519 ? GROUP_SECP256R1 : GROUP_X25519));
    return w.ok();
}
static bool ext_sig_algs(Writer& w) {
    static const xt_u16 a[] = {0x0804, 0x0809, 0x0403};
    w.put16(0x000d);
    w.put16(2 + sizeof(a));
    w.put16((xt_u16)sizeof(a));
    for (size_t i = 0; i < sizeof(a) / sizeof(a[0]); ++i)
        w.put16(a[i]);
    return w.ok();
}
static bool ext_sig_algs_cert(Writer& w) {
    // RFC 8446 signature_algorithms_cert: the verifier supports certificate
    // signatures RSA PKCS#1 with SHA-256/SHA-384 and ECDSA P-256 SHA-256.
    // The main signature_algorithms list remains restricted to schemes
    // valid for TLS 1.3 CertificateVerify (RSA-PSS or ECDSA).
    static const xt_u16 accepted[]={0x0501,0x0401,0x0403};
    w.put16(0x0032); // signature_algorithms_cert extension
    w.put16((xt_u16)(2+sizeof(accepted)));
    w.put16((xt_u16)sizeof(accepted));
    for(size_t i=0;i<sizeof(accepted)/sizeof(accepted[0]);++i)
        w.put16(accepted[i]);
    return w.ok();
}
static bool ext_key_share(Writer& w, NamedGroup g, ByteSpan k) {
    w.put16(0x0033);
    w.put16((xt_u16)(4 + 2 + k.size));
    w.put16((xt_u16)(2 + 2 + k.size));
    w.put16((xt_u16)g);
    w.put16((xt_u16)k.size);
    w.bytes(k.data, k.size);
    return w.ok();
}
static bool ext_sni(Writer& w, const char* host) {
    if (!host || !*host)
        return true;
    size_t h = strlen(host);
    if (h > 65530)
        return false;
    w.put16(0x0000);
    w.put16((xt_u16)(5 + h));
    w.put16((xt_u16)(3 + h));
    w.put8(0);
    w.put16((xt_u16)h);
    w.bytes(host, h);
    return w.ok();
}
static bool ext_cookie(Writer& w, ByteSpan cookie) {
    if (!cookie.size)
        return true;
    if (!cookie.data || cookie.size > 65535)
        return false;
    w.put16(0x002c);
    w.put16((xt_u16)(2 + cookie.size));
    w.put16((xt_u16)cookie.size);
    w.bytes(cookie.data, cookie.size);
    return w.ok();
}
static bool ext_alpn(Writer& w, const char* alpn) {
    if (!alpn || !*alpn)
        return true;

    size_t list_size = 0;
    const char* p = alpn;

    while (*p) {
        const char* start = p;
        while (*p && *p != ',')
            ++p;

        size_t length = (size_t)(p - start);
        if (length == 0 || length > 255)
            return false;

        list_size += 1 + length;
        if (list_size > 65535)
            return false;

        if (*p == ',')
            ++p;
    }

    w.put16(0x0010);
    w.put16((xt_u16)(2 + list_size));
    w.put16((xt_u16)list_size);

    p = alpn;
    while (*p) {
        const char* start = p;
        while (*p && *p != ',')
            ++p;

        size_t length = (size_t)(p - start);
        w.put8((xt_u8)length);
        w.bytes(start, length);

        if (*p == ',')
            ++p;
    }

    return w.ok();
}

Error build_client_hello(const ClientHelloParams& p, MutableByteSpan out, size_t* written) {
    if (written)
        *written = 0;
    if (!out.data || !written || p.random32.size != 32 || !p.random32.data ||
        p.session_id.size > 32 || p.key_share.size == 0 || p.key_share.size > 65535)
        return XT_ERR_INVALID_ARGUMENT;
    Writer w(out);
    w.put8(HS_CLIENT_HELLO);
    size_t hs_len = w.size();
    w.put24(0);
    w.put16(0x0303);
    w.bytes(p.random32.data, 32);
    w.put8((xt_u8)p.session_id.size);
    w.bytes(p.session_id.data, p.session_id.size);
    w.put16(2);
    w.put16(TLS_AES_128_GCM_SHA256);
    w.put8(1);
    w.put8(0);
    size_t ext_len = w.size();
    w.put16(0);
    size_t ext_start = w.size();
    if (!ext_sni(w, p.server_name) || !ext_supported_versions(w) ||
        !ext_supported_groups(w, p.group) || !ext_sig_algs(w) ||
        !ext_sig_algs_cert(w) || !ext_key_share(w, p.group, p.key_share) || !ext_cookie(w, p.cookie) ||
        !ext_alpn(w, p.alpn) || !w.ok())
        return XT_ERR_BUFFER_TOO_SMALL;
    size_t exts = w.size() - ext_start;
    if (exts > 65535)
        return XT_ERR_INVALID_ARGUMENT;
    w.patch16(ext_len, (xt_u16)exts);
    size_t body = w.size() - (hs_len + 3);
    if (body > 0xffffff)
        return XT_ERR_INVALID_ARGUMENT;
    w.patch24(hs_len, (xt_u32)body);
    *written = w.size();
    return XT_OK;
}

} // namespace xboxtls
