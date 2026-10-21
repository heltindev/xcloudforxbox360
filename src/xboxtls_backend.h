#pragma once
#include "secure_http.h"

// Xbox 360 native TLS 1.3 integration from the MIT-licensed XboxTLS13 project.
// One HTTPS exchange per FormPost call. No HTTP downgrade, no insecure mode.
// This is NOT an Xbox Cloud Gaming streaming client.
class XboxTlsBackend {
public:
    struct Diagnostic {
        char stage[48];
        int tls_error;
        int wsa_error;
        unsigned int dns_addresses;
        unsigned long link_flags;
        unsigned long address_flags;
    };
    XboxTlsBackend();
    void get_diagnostic(Diagnostic* snapshot) const;
    static bool post_form(void* context,const char* host,const char* path,
                          const char* form,size_t form_bytes,
                          secure_http::Response* result);
    // Distinct JSON-only Xbox service path with EXACT host/path allowlist.
    // Must retain complete TLS certificate-chain + hostname verification.
    static bool post_xbox_json(void* context,const char* host,const char* path,
                               const char* json,size_t json_bytes,
                               secure_http::Response* result);
    static bool cloud_request(void* context,const char* method,
                              const char* host,const char* path,
                              const char* bearer,const char* json,size_t bytes,
                              secure_http::Response* result);
private:
    bool execute(const char* host,const char* path,const char* form,
                 size_t form_bytes,secure_http::Response* response,
                 bool xbox_json,const char* method="POST",
                 const char* bearer=NULL);
    void record(const char* stage,int tls_error);
    Diagnostic diagnostic_;
};
