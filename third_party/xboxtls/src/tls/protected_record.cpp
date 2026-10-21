#include "xboxtls/aes_gcm.h"
#include "xboxtls/endian.h"
#include "xboxtls/hkdf.h"
#include "xboxtls/tls13.h"
#include <stdlib.h>
#include <string.h>

namespace xboxtls {

TrafficKeysAes128Gcm::TrafficKeysAes128Gcm() {
    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));
    sequence_number = 0;
}

Error derive_traffic_keys_sha256(ByteSpan secret, TrafficKeysAes128Gcm* out) {
    if (!out || !secret.data || secret.size != 32)
        return XT_ERR_INVALID_ARGUMENT;
    Error e =
        hkdf_expand_label_sha256(secret, "key", ByteSpan(0, 0), MutableByteSpan(out->key, 16));
    if (e != XT_OK)
        return e;
    e = hkdf_expand_label_sha256(secret, "iv", ByteSpan(0, 0), MutableByteSpan(out->iv, 12));
    if (e != XT_OK)
        return e;
    out->sequence_number = 0;
    return XT_OK;
}

Error protect_record_aes128_gcm(TrafficKeysAes128Gcm* keys, xt_u8 inner_type, ByteSpan content,
                                size_t padding, MutableByteSpan out, size_t* written) {
    if (!keys || !written || (content.size && !content.data) || !out.data)
        return XT_ERR_INVALID_ARGUMENT;
    if (inner_type < 20 || inner_type > 23)
        return XT_ERR_INVALID_ARGUMENT;
    if (content.size > 16384 || padding > 255)
        return XT_ERR_BAD_RECORD;
    size_t inner_len = content.size + 1 + padding, frag_len = inner_len + 16, total = 5 + frag_len;
    if (frag_len > 16640)
        return XT_ERR_BAD_RECORD;
    if (out.size < total)
        return XT_ERR_BUFFER_TOO_SMALL;
    xt_u8* inner = (xt_u8*)malloc(inner_len);
    if (!inner)
        return XT_ERR_CRYPTO;
    if (content.size)
        memcpy(inner, content.data, content.size);
    inner[content.size] = inner_type;
    if (padding)
        memset(inner + content.size + 1, 0, padding);
    out.data[0] = CONTENT_APPLICATION_DATA;
    out.data[1] = 0x03;
    out.data[2] = 0x03;
    out.data[3] = (xt_u8)(frag_len >> 8);
    out.data[4] = (xt_u8)frag_len;
    xt_u8 nonce[12], tag[16];
    make_record_nonce(keys->iv, keys->sequence_number, nonce);
    Error e =
        aes128_gcm_encrypt(keys->key, nonce, ByteSpan(out.data, 5), ByteSpan(inner, inner_len),
                           MutableByteSpan(out.data + 5, inner_len), tag);
    memset(inner, 0, inner_len);
    free(inner);
    if (e != XT_OK)
        return e;
    memcpy(out.data + 5 + inner_len, tag, 16);
    memset(tag, 0, 16);
    memset(nonce, 0, 12);
    ++keys->sequence_number;
    *written = total;
    return XT_OK;
}

Error unprotect_record_aes128_gcm(TrafficKeysAes128Gcm* keys, ByteSpan record, xt_u8* inner_type,
                                  MutableByteSpan plaintext, size_t* written) {
    if (!keys || !inner_type || !written || !record.data || !plaintext.data)
        return XT_ERR_INVALID_ARGUMENT;
    RecordHeader h;
    Error e = parse_record_header(record, &h);
    if (e != XT_OK)
        return e;
    if (h.type != CONTENT_APPLICATION_DATA || h.legacy_version != 0x0303)
        return XT_ERR_BAD_RECORD;
    if (record.size < 5 + (size_t)h.length || h.length < 17)
        return XT_ERR_BAD_RECORD;
    size_t ct_len = (size_t)h.length - 16;
    if (plaintext.size < ct_len)
        return XT_ERR_BUFFER_TOO_SMALL;
    xt_u8 nonce[12];
    make_record_nonce(keys->iv, keys->sequence_number, nonce);
    e = aes128_gcm_decrypt(keys->key, nonce, ByteSpan(record.data, 5),
                           ByteSpan(record.data + 5, ct_len), record.data + 5 + ct_len,
                           MutableByteSpan(plaintext.data, ct_len));
    memset(nonce, 0, 12);
    if (e != XT_OK)
        return e;
    size_t n = ct_len;
    while (n > 0 && plaintext.data[n - 1] == 0)
        --n;
    if (n == 0)
        return XT_ERR_BAD_RECORD;
    xt_u8 t = plaintext.data[n - 1];
    if (t < 20 || t > 23)
        return XT_ERR_BAD_RECORD;
    --n;
    *inner_type = t;
    *written = n;
    ++keys->sequence_number;
    return XT_OK;
}

} // namespace xboxtls
