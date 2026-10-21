#include "xboxtls/endian.h"
#include "xboxtls/tls13.h"
#include <string.h>
namespace xboxtls {
static const xt_u8 HRR_RANDOM[32] = {
    0xcf, 0x21, 0xad, 0x74, 0xe5, 0x9a, 0x61, 0x11, 0xbe, 0x1d, 0x8c, 0x02, 0x1e, 0x65, 0xb8, 0x91,
    0xc2, 0xa2, 0x11, 0x16, 0x7a, 0xbb, 0x8c, 0x5e, 0x07, 0x9e, 0x09, 0xe2, 0xc8, 0xa8, 0x33, 0x9c};
ServerHelloInfo::ServerHelloInfo()
    : session_id_length(0), cipher_suite(0), selected_version(0), group(GROUP_X25519),
      key_share_length(0), hello_retry_request(false), cookie_length(0) {
    memset(random, 0, 32);
    memset(session_id, 0, 32);
    memset(key_share, 0, 65);
    memset(cookie, 0, sizeof(cookie));
}
static xt_u32 be24(const xt_u8* p) {
    return ((xt_u32)p[0] << 16) | ((xt_u32)p[1] << 8) | p[2];
}
Error parse_server_hello(ByteSpan h, ServerHelloInfo* out) {
    if (!out || !h.data)
        return XT_ERR_INVALID_ARGUMENT;
    *out = ServerHelloInfo();
    if (h.size < 4 || h.data[0] != HS_SERVER_HELLO)
        return XT_ERR_BAD_HANDSHAKE;
    xt_u32 body = be24(h.data + 1);
    if (body + 4 != h.size || body < 38)
        return XT_ERR_BAD_HANDSHAKE;
    size_t p = 4;
    if (load_be16(h.data + p) != 0x0303)
        return XT_ERR_BAD_HANDSHAKE;
    p += 2;
    memcpy(out->random, h.data + p, 32);
    out->hello_retry_request = memcmp(out->random, HRR_RANDOM, 32) == 0;
    p += 32;
    if (p >= h.size)
        return XT_ERR_BAD_HANDSHAKE;
    size_t sid = h.data[p++];
    if (sid > 32 || p + sid + 5 > h.size)
        return XT_ERR_BAD_HANDSHAKE;
    out->session_id_length = sid;
    if (sid)
        memcpy(out->session_id, h.data + p, sid);
    p += sid;
    out->cipher_suite = load_be16(h.data + p);
    p += 2;
    if (h.data[p++] != 0)
        return XT_ERR_BAD_HANDSHAKE;
    size_t ext_total = load_be16(h.data + p);
    p += 2;
    if (p + ext_total != h.size)
        return XT_ERR_BAD_HANDSHAKE;
    bool version = false, keyshare = false;
    size_t end = p + ext_total;
    while (p < end) {
        if (p + 4 > end)
            return XT_ERR_BAD_HANDSHAKE;
        xt_u16 type = load_be16(h.data + p), len = load_be16(h.data + p + 2);
        p += 4;
        if (p + len > end)
            return XT_ERR_BAD_HANDSHAKE;
        if (type == 0x002b) {
            if (len != 2)
                return XT_ERR_BAD_HANDSHAKE;
            out->selected_version = load_be16(h.data + p);
            version = true;
        } else if (type == 0x0033) {
            if (out->hello_retry_request) {
                if (len != 2)
                    return XT_ERR_BAD_HANDSHAKE;
                out->group = (NamedGroup)load_be16(h.data + p);
                out->key_share_length = 0;
                keyshare = true;
            } else {
                if (len < 4)
                    return XT_ERR_BAD_HANDSHAKE;
                out->group = (NamedGroup)load_be16(h.data + p);
                size_t kl = load_be16(h.data + p + 2);
                if (kl + 4 != len || kl > sizeof(out->key_share))
                    return XT_ERR_BAD_HANDSHAKE;
                memcpy(out->key_share, h.data + p + 4, kl);
                out->key_share_length = kl;
                keyshare = true;
            }
        } else if (type == 0x002c) {
            if (!out->hello_retry_request || len < 2)
                return XT_ERR_BAD_HANDSHAKE;
            size_t cl = load_be16(h.data + p);
            if (cl + 2 != len || cl > sizeof(out->cookie))
                return XT_ERR_BAD_HANDSHAKE;
            if (cl)
                memcpy(out->cookie, h.data + p + 2, cl);
            out->cookie_length = cl;
        }
        p += len;
    }
    if (!version || out->selected_version != 0x0304 || !keyshare)
        return XT_ERR_BAD_HANDSHAKE;
    if (out->cipher_suite != TLS_AES_128_GCM_SHA256)
        return XT_ERR_UNSUPPORTED;
    if (out->hello_retry_request) {
        if (out->group != GROUP_X25519 && out->group != GROUP_SECP256R1)
            return XT_ERR_UNSUPPORTED;
        return XT_OK;
    }
    if (out->group == GROUP_X25519 && out->key_share_length == 32)
        return XT_OK;
    if (out->group == GROUP_SECP256R1 && out->key_share_length == 65 && out->key_share[0] == 0x04)
        return XT_OK;
    return XT_ERR_UNSUPPORTED;
}
} // namespace xboxtls
