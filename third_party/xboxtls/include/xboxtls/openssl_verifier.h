#ifndef XBOXTLS_OPENSSL_VERIFIER_H
#define XBOXTLS_OPENSSL_VERIFIER_H

#include "authentication.h"

namespace xboxtls {

// Host/reference verifier. Kept separate from the portable XboxTLS core so
// Xbox 360/XDK builds do not depend on OpenSSL.
class OpenSslServerAuthVerifier : public ServerAuthVerifier {
  public:
    OpenSslServerAuthVerifier();
    virtual ~OpenSslServerAuthVerifier();

    // Load the platform's default trust paths (where supported by OpenSSL).
    Error use_default_trust_store();

    // Add a DER-encoded certificate as a trust anchor.
    Error add_trust_anchor(ByteSpan der_certificate);

    virtual Error verify_chain(const CertificateChainView& chain, const char* hostname);
    virtual Error verify_signature(SignatureScheme scheme, const X509CertificateView& leaf,
                                   ByteSpan signed_message, ByteSpan signature);

  private:
    OpenSslServerAuthVerifier(const OpenSslServerAuthVerifier&);
    OpenSslServerAuthVerifier& operator=(const OpenSslServerAuthVerifier&);
    void* store_;
};

} // namespace xboxtls

#endif
