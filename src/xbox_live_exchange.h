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
    CLOUD_LOGIN_COMPLETE=11,
    EXCHANGE_CANCELLED=12
};
// One worker owns these secrets. Never copy them into snapshots, logs or disk.
class CloudCredentials {
public:
    CloudCredentials();
    ~CloudCredentials();
    bool retain(const char* token,const char* region_uri,
                unsigned int seconds,unsigned int now_ms);
    void clear();
    unsigned int remaining_seconds(unsigned int now_ms);
    const char* token_for_worker(unsigned int now_ms);
    const char* host_for_worker(unsigned int now_ms);
private:
    CloudCredentials(const CloudCredentials&);
    CloudCredentials& operator=(const CloudCredentials&);
    char token_[7000];
    char host_[128];
    unsigned int started_ms_;
    unsigned int lifetime_ms_;
};
struct WorkerContext {
    CloudCredentials* credentials;
    unsigned int (*clock_ms)(void* context);
    bool (*cancelled)(void* context);
    void* context;
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
              PostJson callback,void* context,Result* status,
              WorkerContext* worker=NULL);
const char* stage_text(Stage stage);
bool offline_selftest();
}
