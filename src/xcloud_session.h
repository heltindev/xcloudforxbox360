#pragma once
#include <stddef.h>
#include "secure_http.h"

// The HTTPS session control plane only. Provisioned does not mean that a
// WebRTC connection or playable video exists. One background worker owns this
// object; credentials are supplied for each operation and never retained.
namespace xcloud_session {
enum Stage {
    IDLE=0, CREATING, PROVISIONING, QUEUED, READY_TO_CONNECT,
    CONNECTED, PROVISIONED, STOPPED, FAILED
};
enum { MAX_STATE_POLLS=120 };
struct Snapshot {
    Stage stage;
    int http_status;
    int cleanup_http;
    unsigned int polls;
    unsigned int queue_seconds;
    bool session_created;
    bool connect_sent;
    bool cleanup_pending;
    char server_state[48]; // recognized service state, never arbitrary JSON
    char error_code[64]; // fixed local reason or restricted service code only
};
typedef bool (*Request)(void* context,const char* method,const char* host,
                        const char* path,const char* bearer_token,
                        const char* json,size_t bytes,
                        secure_http::Response* response);
// Reject redirects, URL/userinfo injection and paths outside the session API.
// The backend must validate again, before emitting an Authorization header.
bool valid_session_path(const char* path);
bool normalize_session_path(const char* input,char* output,size_t capacity);
bool validate_request(const char* method,const char* host,const char* path);
const char* stage_text(Stage stage);
bool offline_selftest();
class Session {
public:
    Session();
    ~Session();
    void attach(Request callback,void* context);
    bool start(const char* region_host,const char* gs_token,
               const char* title_id="FORTNITE");
    bool poll(const char* gs_token);
    // Only a token explicitly acquired for console transfer may be used here.
    // Do not pass a gsToken, XSTS token or generic OAuth access token.
    bool connect(const char* gs_token,const char* transfer_token);
    // Idempotent; only removes the exact session returned by this instance.
    // A failed operation retains the path so the caller can still clean up.
    bool stop(const char* gs_token);
    bool needs_cleanup() const;
    const Snapshot& snapshot() const;
private:
    Session(const Session&);
    Session& operator=(const Session&);
    bool request(const char* method,const char* suffix,const char* token,
                 const char* json,size_t bytes,secure_http::Response* response);
    bool fail(const char* reason);
    Request callback_;
    void* context_;
    char host_[128];
    char path_[64];
    Snapshot state_;
};
}
