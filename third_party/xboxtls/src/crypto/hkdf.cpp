#include "xboxtls/hkdf.h"
#include "xboxtls/endian.h"
#include "xboxtls/sha256.h"
#include <string.h>

namespace xboxtls {

void hmac_sha256(ByteSpan key, ByteSpan data, xt_u8 out[32]) {
    xt_u8 k0[64];
    memset(k0, 0, sizeof(k0));
    if (key.size > 64) {
        sha256(key, k0);
    } else if (key.size) {
        memcpy(k0, key.data, key.size);
    }
    xt_u8 ipad[64], opad[64];
    for (int i = 0; i < 64; ++i) {
        ipad[i] = (xt_u8)(k0[i] ^ 0x36);
        opad[i] = (xt_u8)(k0[i] ^ 0x5c);
    }
    Sha256 inner;
    inner.update(ipad, 64);
    if (data.size)
        inner.update(data.data, data.size);
    xt_u8 ih[32];
    inner.final(ih);
    Sha256 outer;
    outer.update(opad, 64);
    outer.update(ih, 32);
    outer.final(out);
}

void hkdf_extract_sha256(ByteSpan salt, ByteSpan ikm, xt_u8 out_prk[32]) {
    xt_u8 zero[32];
    memset(zero, 0, sizeof(zero));
    ByteSpan s = salt.size ? salt : ByteSpan(zero, sizeof(zero));
    hmac_sha256(s, ikm, out_prk);
}

Error hkdf_expand_sha256(ByteSpan prk, ByteSpan info, MutableByteSpan out) {
    if (out.size > 255U * 32U)
        return XT_ERR_INVALID_ARGUMENT;
    xt_u8 t[32];
    size_t tlen = 0, pos = 0;
    xt_u8 ctr = 1;
    while (pos < out.size) {
        xt_u8 buf[32 + 512 + 1];
        if (info.size > 512)
            return XT_ERR_INVALID_ARGUMENT;
        size_t n = 0;
        if (tlen) {
            memcpy(buf, t, tlen);
            n += tlen;
        }
        if (info.size) {
            memcpy(buf + n, info.data, info.size);
            n += info.size;
        }
        buf[n++] = ctr++;
        hmac_sha256(prk, ByteSpan(buf, n), t);
        tlen = 32;
        size_t take = out.size - pos;
        if (take > 32)
            take = 32;
        memcpy(out.data + pos, t, take);
        pos += take;
    }
    return XT_OK;
}

Error hkdf_expand_label_sha256(ByteSpan secret, const char* label, ByteSpan context,
                               MutableByteSpan out) {
    if (!label)
        return XT_ERR_INVALID_ARGUMENT;
    const char* prefix = "tls13 ";
    size_t plen = 6, llen = strlen(label);
    if (plen + llen > 255 || context.size > 255)
        return XT_ERR_INVALID_ARGUMENT;
    xt_u8 info[2 + 1 + 255 + 1 + 255];
    size_t n = 0;
    store_be16(info + n, (xt_u16)out.size);
    n += 2;
    info[n++] = (xt_u8)(plen + llen);
    memcpy(info + n, prefix, plen);
    n += plen;
    memcpy(info + n, label, llen);
    n += llen;
    info[n++] = (xt_u8)context.size;
    if (context.size) {
        memcpy(info + n, context.data, context.size);
        n += context.size;
    }
    return hkdf_expand_sha256(secret, ByteSpan(info, n), out);
}

} // namespace xboxtls
