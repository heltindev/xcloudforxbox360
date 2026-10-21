#ifndef XBOXTLS_NATIVE_VERIFIER_H
#define XBOXTLS_NATIVE_VERIFIER_H

#include "authentication.h"
#include "platform.h"

namespace xboxtls {

class NativeServerAuthVerifier : public ServerAuthVerifier {
  public:
    explicit NativeServerAuthVerifier(Platform* platform);
    Error add_trust_anchor(ByteSpan der_certificate);
    Error clear_trust_anchors();
    Error load_trust_store(ByteSpan compact_store, size_t* loaded_count);
    Error set_leaf_certificate_sha256_pin(ByteSpan pin32);
    void clear_leaf_certificate_sha256_pin();

    virtual Error verify_chain(const CertificateChainView& chain, const char* hostname);
    virtual Error verify_signature(SignatureScheme scheme, const X509CertificateView& leaf,
                                   ByteSpan signed_message, ByteSpan signature);

  private:
    Platform* platform_;
    ByteSpan anchors_[16];
    size_t anchor_count_;
    xt_u8 leaf_certificate_sha256_pin_[32];
    bool leaf_certificate_sha256_pin_enabled_;
};

// Non-sensitive progress marker for diagnosing rejected TLS server certificates.
// Markers never contain credentials, user codes, certificate material or tokens.
void native_auth_set_step(int step);
int native_auth_verify_step();

Error native_rsa_pss_sha256_verify(ByteSpan rsa_public_key_der, ByteSpan message,
                                   ByteSpan signature);
Error native_rsa_pkcs1_sha256_verify(ByteSpan rsa_public_key_der, ByteSpan message,
                                     ByteSpan signature);
Error native_rsa_pkcs1_sha384_verify(ByteSpan rsa_public_key_der, ByteSpan message,
                                     ByteSpan signature);
Error native_p256_ecdsa_sha256_verify(ByteSpan uncompressed_public_key, ByteSpan message,
                                      ByteSpan signature);

} // namespace xboxtls

#endif
