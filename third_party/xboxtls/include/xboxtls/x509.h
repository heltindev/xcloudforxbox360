#ifndef XBOXTLS_X509_H
#define XBOXTLS_X509_H

#include "error.h"
#include "types.h"

namespace xboxtls {

enum PublicKeyKind { PUBLIC_KEY_UNKNOWN = 0, PUBLIC_KEY_RSA = 1, PUBLIC_KEY_EC_P256 = 2 };

struct X509CertificateView {
    ByteSpan der;
    ByteSpan tbs_certificate;
    ByteSpan issuer_name;
    ByteSpan subject_name;
    ByteSpan subject_public_key_info;
    ByteSpan subject_public_key_bits;
    ByteSpan subject_alt_name;
    PublicKeyKind public_key_kind;
    bool validity_present;
    xt_u64 not_before;
    xt_u64 not_after;
    bool basic_constraints_present;
    bool is_ca;
    bool path_len_present;
    xt_u32 path_len_constraint;
    bool key_usage_present;
    bool key_usage_digital_signature;
    bool key_usage_key_cert_sign;
    bool extended_key_usage_present;
    bool eku_server_auth;
    X509CertificateView();
};

Error x509_parse_certificate(ByteSpan der, X509CertificateView* out);
Error x509_verify_hostname(const X509CertificateView& cert, const char* hostname);

} // namespace xboxtls

#endif
