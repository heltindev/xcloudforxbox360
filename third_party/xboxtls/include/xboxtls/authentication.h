#ifndef XBOXTLS_AUTHENTICATION_H
#define XBOXTLS_AUTHENTICATION_H

#include "error.h"
#include "tls13.h"
#include "types.h"
#include "x509.h"

namespace xboxtls {

enum SignatureScheme {
    SIG_ECDSA_SECP256R1_SHA256 = 0x0403,
    SIG_RSA_PSS_RSAE_SHA256 = 0x0804,
    SIG_RSA_PSS_PSS_SHA256 = 0x0809
};

struct CertificateChainEntry {
    ByteSpan der;
    ByteSpan extensions;
};

struct CertificateChainView {
    CertificateChainEntry entries[8];
    size_t count;
    CertificateChainView();
};

class ServerAuthVerifier {
  public:
    virtual ~ServerAuthVerifier() {}
    virtual Error verify_chain(const CertificateChainView& chain, const char* hostname) = 0;
    virtual Error verify_signature(SignatureScheme scheme, const X509CertificateView& leaf,
                                   ByteSpan signed_message, ByteSpan signature) = 0;
};

Error parse_certificate_chain(const CertificateInfo& info, CertificateChainView* out);
Error build_server_certificate_verify_message(ByteSpan transcript_hash, MutableByteSpan out,
                                              size_t* written);
Error authenticate_server_flight(const CertificateInfo& certificates,
                                 const CertificateVerifyInfo& certificate_verify,
                                 ByteSpan transcript_hash_before_certificate_verify,
                                 const char* hostname, ServerAuthVerifier* verifier);

} // namespace xboxtls

#endif
