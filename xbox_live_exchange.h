#pragma once
#include <stddef.h>
#include "secure_http.h"

// Native Xbox Live token exchange stage (not xCloud playback).
// A service-verified Microsoft OAuth token is required from the owner's
// application. The HTTPS backend must validate the endpoint certificate,
// hostname and CA chain. The result NEVER contains credentials.
namespace xbox_live_exchange {
enum Stage {
    NOT_STARTED=0,
    XBOX_SCOPE_MISSING=1,
    INVALID_TOKEN=2,
    HTTPS_UNAVAILABLE=3,
    XBOX_USER_HTTP_REJECTED=4,
    XBOX_USER_RESPONSE_INVALID=5,
    XSTS_HTTP_REJECTED=6,
    XSTS_RESPONSE_INVALID=7,
    XSTS_COMPLETE=8,
    CLOUD_HTTP_REJECTED=9,
    CLOUD_RESPONSE_INVALID=10,
    CLOUD_LOGIN_COMPLETE=11
};
struct Result {
    Stage stage;
    bool xbox_user_received;
    bool xsts_received;
    bool cloud_login_received;
    unsigned int cloud_regions;
    int cloud_http;
    int user_http;
    int xsts_http;
    unsigned int xerr; // numeric Xbox error only; no user or token data
};
typedef bool (*PostJson)(void* context,const char* host,
                         const char* path,const char* json,size_t bytes,
                         secure_http::Response* response);
// Actual HTTPS callback is injectable to permit deterministic offline tests.
// Network side-effects must only be invoked on a background worker.
bool exchange(const char* msa_token,bool xbox_scope_verified,
              PostJson callback,void* context,Result* status);
const char* stage_text(Stage stage);
bool offline_selftest();
}
