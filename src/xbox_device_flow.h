#pragma once
#include "secure_http.h"
#include "oauth_device.h"

// A fully bounded OAuth 2.0 Device Authorization state machine for a
// *registered* public client. Based on Microsoft's documented protocol,
// and inspired by GR33N's asynchronous request/poll architecture.
//
// This module does not provide TLS or access to xCloud. No Microsoft
// password is ever requested. Runtime tokens are wiped; an optional worker-only
// callback may persist the refresh token through protected platform storage.
namespace xbox_device_flow {
bool offline_selftest();

enum State {
    MISSING_CONFIG=0, TRANSPORT_REQUIRED=1, READY=2,
    REQUESTING=3, WAITING_USER=4, AUTHORIZED=5,
    EXPIRED=6, DECLINED=7, FAILED=8
};
typedef bool (*PersistRefresh)(void*,const char* client,const char* requested,
                               const char* granted,const char* refresh);
class Flow {
public:
    Flow();
    ~Flow();
    // The public application ID must belong to the actual registered app;
    // do not borrow a third-party Xbox client ID.
    bool configure(const char* client_id,const char* scopes,
                   const secure_http::Transport* transport);
    State state() const {return state_;}
    const char* verification_url() const {return info_.verification_uri;}
    const char* user_code() const {return info_.user_code;}
    unsigned int interval_seconds() const {return info_.interval;}
    unsigned int remaining_seconds(unsigned long now_ms) const;
    unsigned int next_poll_seconds(unsigned long now_ms) const;
    unsigned int poll_attempts() const {return poll_attempts_;}
    int last_http_status() const {return http_status_;}
    const char* status_text() const;
    bool start(unsigned long now_ms);
    void set_persistence(PersistRefresh callback,void* context);
    bool resume_from_refresh(const char* refresh,const char* granted,unsigned long now_ms);
    bool reauthentication_required() const {return reauth_required_;}
    // Rebase expiration and first poll to the actual HTTP response arrival.
    // This avoids an immediate poll when the TLS request took several seconds.
    void code_received_at(unsigned long now_ms);
    bool poll(unsigned long now_ms);
    // Rebase interval after an actual blocking HTTPS poll, not at its start.
    void poll_completed_at(unsigned long now_ms);
    // No tokens exposed via UI/logs. A future Xbox Live/XSTS module must
    // explicitly request authorized access after HTTPS works.
    bool is_authorized() const {return state_==AUTHORIZED;}
    bool xbox_scope_granted() const; // Only if OAuth response explicitly says so.
    // Borrowed pointer for the SAME worker thread only, valid until the
    // next Flow mutation/destruction. Do not log/store/copy to UI state.
    const char* xbox_token_for_worker() const;
    // Same worker only; refresh rotation stays in RAM and keeps consent scope.
    bool refresh_for_worker(unsigned long now_ms);
    bool request_transfer_token(char* output,size_t capacity);
    void cancel();
private:
    Flow(const Flow&);
    Flow& operator=(const Flow&);
    bool post(const char* path,const char* form,secure_http::Response* result);
    bool handle_token_reply(const secure_http::Response& reply,
                            unsigned long now_ms);
    void fail();
    void persist_refresh();
    bool renew(bool transfer,unsigned long now_ms,char* output,size_t capacity);
    PersistRefresh persistence_;
    void* persistence_context_;
    bool reauth_required_;
    const secure_http::Transport* transport_;
    State state_;
    oauth_device::DeviceAuthorization info_;
    char client_id_[64];
    char scopes_[256];
    char access_token_[12288];
    char refresh_token_[8192];
    char granted_scopes_[256];
    unsigned long next_poll_ms_;
    unsigned long expires_at_ms_;
    unsigned int token_expires_s_;
    unsigned int poll_attempts_;
    int http_status_;
    char oauth_error_[96]; // server error code only; never description/token
};
}
