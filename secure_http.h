#pragma once
#include <stddef.h>

// All account operations MUST use a TLS peer validated by a trusted
// certificate chain + expected server hostname. There is intentionally no
// insecure/plain-HTTP fallback (not even when XNet is broken).
namespace secure_http {
enum { RESPONSE_CAP=32768, ERROR_CAP=160 };
struct Response {
    int http_status;
    size_t length;
    bool peer_certificate_verified;
    bool hostname_verified;
    char body[RESPONSE_CAP];
};
typedef bool (*FormPost)(void* context,const char* host,const char* path,
                         const char* form,size_t form_bytes,Response* result);
class Transport {
public:
    Transport();
    void attach(FormPost validated_tls_post,void* context);
    bool configured() const;
    // Returns false on absent backend, failed TLS, malformed response or
    // unverified peer. Result buffer is caller-owned, including secret tokens.
    bool post_form(const char* host,const char* path,
                   const char* body,size_t count,Response* response) const;
private:
    FormPost callback_;
    void* context_;
};
void secure_zero(void* address,size_t bytes);
}
