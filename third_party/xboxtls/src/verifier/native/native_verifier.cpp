#include "xboxtls/native_verifier.h"
#include "xboxtls/trust_store.h"
#include <string.h>

namespace xboxtls {

static volatile int g_native_auth_step=0;
void native_auth_set_step(int step){g_native_auth_step=step;}
int native_auth_verify_step(){return g_native_auth_step;}

struct Tlv {
    xt_u8 tag;
    ByteSpan full;
    ByteSpan value;
    size_t next;
};
static Error tlv(ByteSpan in, size_t off, Tlv* o) {
    if (!o || !in.data || off >= in.size)
        return XT_ERR_BAD_HANDSHAKE;
    size_t p = off;
    o->tag = in.data[p++];
    if (p >= in.size)
        return XT_ERR_BAD_HANDSHAKE;
    xt_u8 lb = in.data[p++];
    size_t len = 0;
    if (!(lb & 0x80))
        len = lb;
    else {
        size_t n = lb & 0x7f;
        if (!n || n > 4 || p + n > in.size || in.data[p] == 0)
            return XT_ERR_BAD_HANDSHAKE;
        for (size_t i = 0; i < n; ++i)
            len = (len << 8) | in.data[p++];
        if (len < 128)
            return XT_ERR_BAD_HANDSHAKE;
    }
    if (len > in.size - p)
        return XT_ERR_BAD_HANDSHAKE;
    o->full = ByteSpan(in.data + off, p - off + len);
    o->value = ByteSpan(in.data + p, len);
    o->next = p + len;
    return XT_OK;
}
static bool oid_eq(ByteSpan a, const xt_u8* b, size_t n) {
    return a.size == n && memcmp(a.data, b, n) == 0;
}

enum CertSigKind {
    CERT_SIG_UNKNOWN = 0,
    CERT_SIG_RSA_SHA256 = 1,
    CERT_SIG_ECDSA_SHA256 = 2,
    CERT_SIG_RSA_PSS_SHA256 = 3,
    CERT_SIG_RSA_SHA384 = 4
};
struct CertSig {
    ByteSpan tbs;
    ByteSpan sig;
    CertSigKind kind;
};
static bool parse_alg_oid(ByteSpan alg_full, ByteSpan* oid_value, ByteSpan* params) {
    if (!oid_value || !params)
        return false;
    Tlv alg;
    if (tlv(alg_full, 0, &alg) != XT_OK || alg.tag != 0x30 || alg.next != alg_full.size)
        return false;
    Tlv oid;
    if (tlv(alg.value, 0, &oid) != XT_OK || oid.tag != 0x06)
        return false;
    *oid_value = oid.value;
    if (oid.next == alg.value.size) {
        *params = ByteSpan();
        return true;
    }
    Tlv p;
    if (tlv(alg.value, oid.next, &p) != XT_OK || p.next != alg.value.size)
        return false;
    *params = p.full;
    return true;
}
static bool is_sha256_alg(ByteSpan alg_full) {
    ByteSpan oid, params;
    if (!parse_alg_oid(alg_full, &oid, &params))
        return false;
    static const xt_u8 SHA256[] = {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01};
    if (!oid_eq(oid, SHA256, sizeof(SHA256)))
        return false;
    if (!params.size)
        return true;
    return params.size == 2 && params.data[0] == 0x05 && params.data[1] == 0x00;
}
static bool parse_pss_sha256_params(ByteSpan params_full) {
    if (!params_full.data || !params_full.size)
        return false;
    Tlv seq;
    if (tlv(params_full, 0, &seq) != XT_OK || seq.tag != 0x30 || seq.next != params_full.size)
        return false;
    bool hash = false, mgf = false, salt = false;
    size_t p = 0;
    while (p < seq.value.size) {
        Tlv f;
        if (tlv(seq.value, p, &f) != XT_OK)
            return false;
        if (f.tag == 0xA0) {
            Tlv a;
            if (tlv(f.value, 0, &a) != XT_OK || a.next != f.value.size || !is_sha256_alg(a.full))
                return false;
            hash = true;
        } else if (f.tag == 0xA1) {
            Tlv a;
            if (tlv(f.value, 0, &a) != XT_OK || a.tag != 0x30 || a.next != f.value.size)
                return false;
            ByteSpan oid, par;
            if (!parse_alg_oid(a.full, &oid, &par))
                return false;
            static const xt_u8 MGF1[] = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x08};
            if (!oid_eq(oid, MGF1, sizeof(MGF1)) || !is_sha256_alg(par))
                return false;
            mgf = true;
        } else if (f.tag == 0xA2) {
            Tlv i;
            if (tlv(f.value, 0, &i) != XT_OK || i.tag != 0x02 || i.next != f.value.size ||
                i.value.size != 1 || i.value.data[0] != 32)
                return false;
            salt = true;
        } else if (f.tag == 0xA3) {
            Tlv i;
            if (tlv(f.value, 0, &i) != XT_OK || i.tag != 0x02 || i.next != f.value.size ||
                i.value.size != 1 || i.value.data[0] != 1)
                return false;
        } else
            return false;
        p = f.next;
    }
    return hash && mgf && salt;
}
static Error cert_signature(ByteSpan der, CertSig* out) {
    if (!out)
        return XT_ERR_INVALID_ARGUMENT;
    Tlv cert;
    Error e = tlv(der, 0, &cert);
    if (e != XT_OK || cert.tag != 0x30 || cert.next != der.size)
        return XT_ERR_BAD_HANDSHAKE;
    Tlv tbs;
    e = tlv(cert.value, 0, &tbs);
    if (e != XT_OK || tbs.tag != 0x30)
        return XT_ERR_BAD_HANDSHAKE;
    Tlv alg;
    e = tlv(cert.value, tbs.next, &alg);
    if (e != XT_OK || alg.tag != 0x30)
        return XT_ERR_BAD_HANDSHAKE;
    // RFC 5280 requires the TBSCertificate signature AlgorithmIdentifier to match the outer one.
    size_t tp = 0;
    Tlv f;
    e = tlv(tbs.value, tp, &f);
    if (e != XT_OK)
        return e;
    if (f.tag == 0xA0)
        tp = f.next;
    Tlv serial;
    e = tlv(tbs.value, tp, &serial);
    if (e != XT_OK || serial.tag != 0x02)
        return XT_ERR_BAD_HANDSHAKE;
    Tlv tbs_alg;
    e = tlv(tbs.value, serial.next, &tbs_alg);
    if (e != XT_OK || tbs_alg.tag != 0x30)
        return XT_ERR_BAD_HANDSHAKE;
    if (tbs_alg.full.size != alg.full.size ||
        memcmp(tbs_alg.full.data, alg.full.data, alg.full.size) != 0)
        return XT_ERR_VERIFY;
    ByteSpan oidv, params;
    if (!parse_alg_oid(alg.full, &oidv, &params))
        return XT_ERR_BAD_HANDSHAKE;
    static const xt_u8 SHA256_RSA[] = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B};
    static const xt_u8 SHA384_RSA[] = {0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,0x01,0x0C};
    static const xt_u8 SHA256_ECDSA[] = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02};
    static const xt_u8 RSA_PSS[] = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0A};
    Tlv sig;
    e = tlv(cert.value, alg.next, &sig);
    if (e != XT_OK || sig.tag != 0x03 || sig.next != cert.value.size || sig.value.size < 1 ||
        sig.value.data[0] != 0)
        return XT_ERR_BAD_HANDSHAKE;
    out->tbs = tbs.full;
    out->sig = ByteSpan(sig.value.data + 1, sig.value.size - 1);
    out->kind = CERT_SIG_UNKNOWN;
    if (oid_eq(oidv, SHA256_RSA, sizeof(SHA256_RSA)))
        out->kind = CERT_SIG_RSA_SHA256;
    else if (oid_eq(oidv, SHA384_RSA, sizeof(SHA384_RSA)))
        out->kind = CERT_SIG_RSA_SHA384;
    else if (oid_eq(oidv, SHA256_ECDSA, sizeof(SHA256_ECDSA)))
        out->kind = CERT_SIG_ECDSA_SHA256;
    else if (oid_eq(oidv, RSA_PSS, sizeof(RSA_PSS)) && parse_pss_sha256_params(params))
        out->kind = CERT_SIG_RSA_PSS_SHA256;
    return XT_OK;
}

NativeServerAuthVerifier::NativeServerAuthVerifier(Platform* p)
    : platform_(p), anchor_count_(0), leaf_certificate_sha256_pin_enabled_(false) {
    memset(anchors_, 0, sizeof(anchors_));
    memset(leaf_certificate_sha256_pin_, 0, sizeof(leaf_certificate_sha256_pin_));
}

Error NativeServerAuthVerifier::set_leaf_certificate_sha256_pin(ByteSpan pin32) {
    if (!pin32.data || pin32.size != 32)
        return XT_ERR_INVALID_ARGUMENT;

    memcpy(leaf_certificate_sha256_pin_, pin32.data, 32);
    leaf_certificate_sha256_pin_enabled_ = true;
    return XT_OK;
}

void NativeServerAuthVerifier::clear_leaf_certificate_sha256_pin() {
    memset(leaf_certificate_sha256_pin_, 0, sizeof(leaf_certificate_sha256_pin_));
    leaf_certificate_sha256_pin_enabled_ = false;
}

Error NativeServerAuthVerifier::add_trust_anchor(ByteSpan der) {
    if (!der.data || !der.size)
        return XT_ERR_INVALID_ARGUMENT;
    if (anchor_count_ >= 16)
        return XT_ERR_BUFFER_TOO_SMALL;
    X509CertificateView v;
    Error e = x509_parse_certificate(der, &v);
    if (e != XT_OK)
        return e;
    anchors_[anchor_count_++] = der;
    return XT_OK;
}
Error NativeServerAuthVerifier::clear_trust_anchors() {
    anchor_count_ = 0;
    memset(anchors_, 0, sizeof(anchors_));
    return XT_OK;
}
Error NativeServerAuthVerifier::load_trust_store(ByteSpan store, size_t* loaded) {
    return load_compact_trust_store(store, this, loaded);
}

static Error verify_cert_signed_by(ByteSpan child, ByteSpan issuer) {
    X509CertificateView iv;
    Error e = x509_parse_certificate(issuer, &iv);
    if (e != XT_OK)
        return e;
    CertSig cs;
    e = cert_signature(child, &cs);
    if (e != XT_OK)
        return e;
    if (cs.kind == CERT_SIG_RSA_SHA256 && iv.public_key_kind == PUBLIC_KEY_RSA)
        return native_rsa_pkcs1_sha256_verify(iv.subject_public_key_bits, cs.tbs, cs.sig);
    if (cs.kind == CERT_SIG_RSA_SHA384 && iv.public_key_kind == PUBLIC_KEY_RSA)
        return native_rsa_pkcs1_sha384_verify(iv.subject_public_key_bits, cs.tbs, cs.sig);
    if (cs.kind == CERT_SIG_RSA_PSS_SHA256 && iv.public_key_kind == PUBLIC_KEY_RSA)
        return native_rsa_pss_sha256_verify(iv.subject_public_key_bits, cs.tbs, cs.sig);
    if (cs.kind == CERT_SIG_ECDSA_SHA256 && iv.public_key_kind == PUBLIC_KEY_EC_P256)
        return native_p256_ecdsa_sha256_verify(iv.subject_public_key_bits, cs.tbs, cs.sig);
    return XT_ERR_UNSUPPORTED;
}

Error NativeServerAuthVerifier::verify_chain(const CertificateChainView& chain,
                                             const char* hostname) {
    native_auth_set_step(100); // Verify-chain entry
    if (!hostname || !*hostname || chain.count == 0 || anchor_count_ == 0)
        return XT_ERR_VERIFY;
    X509CertificateView leaf;
    Error e = x509_parse_certificate(chain.entries[0].der, &leaf);
    if (e != XT_OK)
        return e;

    if (leaf_certificate_sha256_pin_enabled_) {
        xt_u8 digest[32];
        sha256(chain.entries[0].der, digest);

        xt_u8 difference = 0;
        for (size_t i = 0; i < sizeof(digest); ++i)
            difference |= (xt_u8)(digest[i] ^ leaf_certificate_sha256_pin_[i]);

        memset(digest, 0, sizeof(digest));

        if (difference != 0)
            return XT_ERR_VERIFY;
    }
    native_auth_set_step(110); // Leaf constraints and time
    if (!leaf.validity_present || (leaf.basic_constraints_present && leaf.is_ca))
        return XT_ERR_VERIFY;
    if (leaf.key_usage_present && !leaf.key_usage_digital_signature)
        return XT_ERR_VERIFY;
    if (leaf.extended_key_usage_present && !leaf.eku_server_auth)
        return XT_ERR_VERIFY;
    if (platform_) {
        xt_u64 now = platform_->unix_time_seconds();
        if (now < leaf.not_before || now > leaf.not_after)
            return XT_ERR_VERIFY;
    }
    native_auth_set_step(120); // Leaf hostname
    e = x509_verify_hostname(leaf, hostname);
    if (e != XT_OK)
        return XT_ERR_VERIFY;
    native_auth_set_step(130); // Chain intermediate validation
    for (size_t i = 0; i + 1 < chain.count; ++i) {
        X509CertificateView child, issuer;
        if (x509_parse_certificate(chain.entries[i].der, &child) != XT_OK ||
            x509_parse_certificate(chain.entries[i + 1].der, &issuer) != XT_OK)
            return XT_ERR_VERIFY;
        if (child.issuer_name.size != issuer.subject_name.size ||
            memcmp(child.issuer_name.data, issuer.subject_name.data, child.issuer_name.size) != 0)
            return XT_ERR_VERIFY;
        if (!issuer.basic_constraints_present || !issuer.is_ca ||
            (issuer.key_usage_present && !issuer.key_usage_key_cert_sign))
            return XT_ERR_VERIFY;
        // Conservative pathLenConstraint enforcement: every CA certificate below this issuer
        // counts.
        if (issuer.path_len_present && issuer.path_len_constraint < (xt_u32)i)
            return XT_ERR_VERIFY;
        if (platform_) {
            xt_u64 now = platform_->unix_time_seconds();
            if (!issuer.validity_present || now < issuer.not_before || now > issuer.not_after)
                return XT_ERR_VERIFY;
        }
        native_auth_set_step(140); // RSA/X.509 chain signature
        e = verify_cert_signed_by(chain.entries[i].der, chain.entries[i + 1].der);
        if (e != XT_OK)
            return XT_ERR_VERIFY;
    }
    ByteSpan top = chain.entries[chain.count - 1].der;
    X509CertificateView topv;
    if (x509_parse_certificate(top, &topv) != XT_OK)
        return XT_ERR_VERIFY;
    native_auth_set_step(150); // Root chain to trusted anchor
    for (size_t a = 0; a < anchor_count_; ++a) {
        X509CertificateView av;
        if (x509_parse_certificate(anchors_[a], &av) != XT_OK)
            continue;
        if (!av.basic_constraints_present || !av.is_ca ||
            (av.key_usage_present && !av.key_usage_key_cert_sign))
            continue;
        if (platform_) {
            xt_u64 now = platform_->unix_time_seconds();
            if (!av.validity_present || now < av.not_before || now > av.not_after)
                continue;
        }
        if (top.size == anchors_[a].size && memcmp(top.data, anchors_[a].data, top.size) == 0) {
            const size_t ca_below = chain.count >= 2 ? chain.count - 2 : 0;
            if (av.path_len_present && av.path_len_constraint < (xt_u32)ca_below)
                continue;
            return XT_OK;
        }
        if (topv.issuer_name.size != av.subject_name.size ||
            memcmp(topv.issuer_name.data, av.subject_name.data, topv.issuer_name.size) != 0)
            continue;
        const size_t ca_below = chain.count >= 1 ? chain.count - 1 : 0;
        if (av.path_len_present && av.path_len_constraint < (xt_u32)ca_below)
            continue;
        native_auth_set_step(160); // Root cross-signature
        if (verify_cert_signed_by(top, anchors_[a]) == XT_OK) {
            native_auth_set_step(200);
            return XT_OK;
        }
    }
    return XT_ERR_VERIFY;
}

Error NativeServerAuthVerifier::verify_signature(SignatureScheme scheme,
                                                 const X509CertificateView& leaf, ByteSpan msg,
                                                 ByteSpan sig) {
    if (!msg.data || !sig.data)
        return XT_ERR_INVALID_ARGUMENT;
    if ((scheme == SIG_RSA_PSS_RSAE_SHA256 || scheme == SIG_RSA_PSS_PSS_SHA256) &&
        leaf.public_key_kind == PUBLIC_KEY_RSA)
        return native_rsa_pss_sha256_verify(leaf.subject_public_key_bits, msg, sig);
    if (scheme == SIG_ECDSA_SECP256R1_SHA256 && leaf.public_key_kind == PUBLIC_KEY_EC_P256)
        return native_p256_ecdsa_sha256_verify(leaf.subject_public_key_bits, msg, sig);
    return XT_ERR_VERIFY;
}

} // namespace xboxtls
