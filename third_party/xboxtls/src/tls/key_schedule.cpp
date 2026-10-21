#include "xboxtls/hkdf.h"
#include "xboxtls/tls13.h"
#include <string.h>

namespace xboxtls {

KeyScheduleSha256::KeyScheduleSha256() {
    memset(this, 0, sizeof(*this));
}

static Error derive_secret(ByteSpan secret, const char* label, ByteSpan th, xt_u8 out[32]) {
    return hkdf_expand_label_sha256(secret, label, th, MutableByteSpan(out, 32));
}

Error KeyScheduleSha256::derive_early_secret(ByteSpan psk) {
    // RFC 8446 Section 7.1: for non-PSK handshakes the input key material
    // MUST be a Hash.length-long zero string, NOT a zero-length span.
    // HMAC(zero-salt, empty-data) produces incorrect TLS traffic keys.
    xt_u8 zeros[32];
    memset(zeros, 0, sizeof(zeros));
    ByteSpan ikm = psk.size ? psk : ByteSpan(zeros, sizeof(zeros));
    hkdf_extract_sha256(ByteSpan(zeros, sizeof(zeros)), ikm, early_secret);
    memset(zeros, 0, sizeof(zeros));
    return XT_OK;
}

Error KeyScheduleSha256::derive_handshake_secret(ByteSpan ecdhe, ByteSpan th) {
    xt_u8 empty_hash[32];
    sha256(ByteSpan(0, 0), empty_hash);
    xt_u8 derived[32];
    Error e =
        derive_secret(ByteSpan(early_secret, 32), "derived", ByteSpan(empty_hash, 32), derived);
    if (e != XT_OK)
        return e;
    hkdf_extract_sha256(ByteSpan(derived, 32), ecdhe, handshake_secret);
    e = derive_secret(ByteSpan(handshake_secret, 32), "c hs traffic", th,
                      client_handshake_traffic_secret);
    if (e != XT_OK)
        return e;
    return derive_secret(ByteSpan(handshake_secret, 32), "s hs traffic", th,
                         server_handshake_traffic_secret);
}

Error KeyScheduleSha256::derive_master_secret(ByteSpan th) {
    xt_u8 empty_hash[32];
    sha256(ByteSpan(0, 0), empty_hash);
    xt_u8 derived[32], zeros[32];
    memset(zeros, 0, 32);
    Error e =
        derive_secret(ByteSpan(handshake_secret, 32), "derived", ByteSpan(empty_hash, 32), derived);
    if (e != XT_OK)
        return e;
    hkdf_extract_sha256(ByteSpan(derived, 32), ByteSpan(zeros, 32), master_secret);
    e = derive_secret(ByteSpan(master_secret, 32), "c ap traffic", th,
                      client_application_traffic_secret);
    if (e != XT_OK)
        return e;
    return derive_secret(ByteSpan(master_secret, 32), "s ap traffic", th,
                         server_application_traffic_secret);
}

Error update_traffic_secret_sha256(xt_u8 traffic_secret[32], TrafficKeysAes128Gcm* keys) {
    if (!traffic_secret || !keys)
        return XT_ERR_INVALID_ARGUMENT;
    xt_u8 next[32];
    Error e = hkdf_expand_label_sha256(ByteSpan(traffic_secret, 32), "traffic upd", ByteSpan(0, 0),
                                       MutableByteSpan(next, sizeof(next)));
    if (e != XT_OK)
        return e;
    for (size_t i = 0; i < 32; ++i)
        traffic_secret[i] = next[i];
    for (size_t i = 0; i < sizeof(next); ++i)
        next[i] = 0;
    return derive_traffic_keys_sha256(ByteSpan(traffic_secret, 32), keys);
}

} // namespace xboxtls
