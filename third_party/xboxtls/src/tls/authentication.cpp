#include "xboxtls/authentication.h"
#include "xboxtls/endian.h"
#include "xboxtls/native_verifier.h"
#include <string.h>

namespace xboxtls {

CertificateChainView::CertificateChainView() : count(0) {}

static xt_u32 read_u24(const xt_u8* p) {
    return ((xt_u32)p[0] << 16) | ((xt_u32)p[1] << 8) | p[2];
}

Error parse_certificate_chain(const CertificateInfo& info, CertificateChainView* out) {
    if (!out)
        return XT_ERR_INVALID_ARGUMENT;
    *out = CertificateChainView();
    ByteSpan s = info.certificate_list;
    size_t p = 0;
    while (p < s.size) {
        if (out->count >= 8 || s.size - p < 3)
            return XT_ERR_BAD_HANDSHAKE;
        size_t cert_len = read_u24(s.data + p);
        p += 3;
        if (cert_len == 0 || cert_len > s.size - p)
            return XT_ERR_BAD_HANDSHAKE;
        ByteSpan der(s.data + p, cert_len);
        p += cert_len;
        if (s.size - p < 2)
            return XT_ERR_BAD_HANDSHAKE;
        size_t ext_len = ((size_t)s.data[p] << 8) | s.data[p + 1];
        p += 2;
        if (ext_len > s.size - p)
            return XT_ERR_BAD_HANDSHAKE;
        out->entries[out->count].der = der;
        out->entries[out->count].extensions = ByteSpan(s.data + p, ext_len);
        ++out->count;
        p += ext_len;
    }
    return out->count ? XT_OK : XT_ERR_VERIFY;
}

Error build_server_certificate_verify_message(ByteSpan transcript_hash, MutableByteSpan out,
                                              size_t* written) {
    static const char context[] = "TLS 1.3, server CertificateVerify";
    if (!written || transcript_hash.size != 32 || !transcript_hash.data)
        return XT_ERR_INVALID_ARGUMENT;
    const size_t need = 64 + sizeof(context) - 1 + 1 + 32;
    if (!out.data || out.size < need)
        return XT_ERR_BUFFER_TOO_SMALL;
    memset(out.data, 0x20, 64);
    memcpy(out.data + 64, context, sizeof(context) - 1);
    out.data[64 + sizeof(context) - 1] = 0;
    memcpy(out.data + 64 + sizeof(context), transcript_hash.data, 32);
    *written = need;
    return XT_OK;
}

Error authenticate_server_flight(const CertificateInfo& certificates,
                                 const CertificateVerifyInfo& certificate_verify,
                                 ByteSpan transcript_hash_before_certificate_verify,
                                 const char* hostname, ServerAuthVerifier* verifier) {
    native_auth_set_step(10); // Start TLS certificate authentication
    if (!verifier || !hostname)
        return XT_ERR_VERIFY;
    CertificateChainView chain;
    Error e = parse_certificate_chain(certificates, &chain);
    if (e != XT_OK)
        return e;
    X509CertificateView leaf;
    e = x509_parse_certificate(chain.entries[0].der, &leaf);
    if (e != XT_OK)
        return e;
    native_auth_set_step(20); // Parse and check TLS leaf hostname
    e = x509_verify_hostname(leaf, hostname);
    if (e != XT_OK)
        return e;
    native_auth_set_step(30); // Start trust chain verification
    e = verifier->verify_chain(chain, hostname);
    if (e != XT_OK)
        return e;

    SignatureScheme scheme;
    if (certificate_verify.algorithm == SIG_ECDSA_SECP256R1_SHA256)
        scheme = SIG_ECDSA_SECP256R1_SHA256;
    else if (certificate_verify.algorithm == SIG_RSA_PSS_RSAE_SHA256)
        scheme = SIG_RSA_PSS_RSAE_SHA256;
    else if (certificate_verify.algorithm == SIG_RSA_PSS_PSS_SHA256)
        scheme = SIG_RSA_PSS_PSS_SHA256;
    else
        return XT_ERR_UNSUPPORTED;
    if ((scheme == SIG_ECDSA_SECP256R1_SHA256 && leaf.public_key_kind != PUBLIC_KEY_EC_P256) ||
        ((scheme == SIG_RSA_PSS_RSAE_SHA256 || scheme == SIG_RSA_PSS_PSS_SHA256) &&
         leaf.public_key_kind != PUBLIC_KEY_RSA))
        return XT_ERR_VERIFY;

    xt_u8 signed_message[160];
    size_t n = 0;
    e = build_server_certificate_verify_message(
        transcript_hash_before_certificate_verify,
        MutableByteSpan(signed_message, sizeof(signed_message)), &n);
    if (e != XT_OK)
        return e;
    native_auth_set_step(180); // Verify TLS CertificateVerify signature
    e=verifier->verify_signature(scheme, leaf, ByteSpan(signed_message, n),
                                 certificate_verify.signature);
    if(e==XT_OK)native_auth_set_step(250);
    return e;
}

} // namespace xboxtls
