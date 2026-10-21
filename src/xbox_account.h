#pragma once
#include <xtl.h>
namespace xbox_device_flow {class Flow;}
namespace xbox_live_exchange {class CloudCredentials;}
namespace xcloud_session {class Session;}
class XboxTlsBackend;

// Asynchronous Microsoft device-code login experiment for one Xbox 360 XEX.
// Callbacks do not run on the D3D render thread. Device code shown only
// after the real Microsoft HTTPS response. No passwords, token persistence,
// account identifiers or automatic account authorization. The app can
// request a public device code after its network probe; confirmation always
// requires the user at Microsoft's site.
class XboxAccount {
public:
    enum Status { IDLE=0, STARTING=1, WAITING_FOR_USER=2,
                  ACCOUNT_AUTHORIZED=3, LOGIN_FAILED=4,
                  USER_CANCELLED=5, NOT_CONFIGURED=6,
                  XBOX_EXCHANGING=7 };
    struct Snapshot {
        Status state;
        char verification_url[256];
        char user_code[64];
        char stage[64];
        int tls_error;
        int wsa_error;
        int http_status;
        unsigned int seconds_left;
        unsigned int poll_attempts;
        unsigned int next_poll_seconds;
        bool xbox_scope_verified;
        bool xbox_user_token_verified;
        bool xsts_token_verified;
        unsigned int xbox_xerr;
        int xbox_http_status;
        int xsts_http_status;
        bool cloud_login_verified;
        int cloud_http_status;
        unsigned int cloud_regions;
        int cache_status;
        bool cache_available;
        bool cache_resumed;
        bool cloud_credentials_ready;
        unsigned int cloud_credentials_seconds;
        unsigned int session_stage;
        int session_http_status;
        int session_cleanup_http;
        unsigned int session_wait_seconds;
        char session_server_state[48];
        char session_error[80];
        bool session_created;
        bool session_closed;
        bool session_test_complete;
        bool session_cleanup_failed;
    };
    XboxAccount();
    ~XboxAccount();
    bool start(const char* public_client_id);
    void cancel();
    bool forget_saved_login();
    Snapshot snapshot();
private:
    static DWORD WINAPI worker_main(LPVOID arg);
    void perform();
    void publish_cache(int status,bool available,bool resumed);
    static bool persist_login(void*,const char*,const char*,const char*,const char*);
    bool erase_saved_login();
    void perform_session(xbox_device_flow::Flow& flow,XboxTlsBackend& backend,
                         xbox_live_exchange::CloudCredentials& credentials);
    void publish_session(const xcloud_session::Session& session,bool complete,
                         const char* error_override=NULL);
    static unsigned int worker_clock(void* context);
    static bool worker_cancelled(void* context);
    void publish_credentials(unsigned int seconds);
    void publish(Status state,const char* stage,
                 const char* url,const char* code,
                 int tls,int wsa,int http,unsigned int remaining,
                 unsigned int attempts=0,unsigned int next_poll=0,
                 bool xbox_scope_verified=false,
                 bool xbox_user_token_verified=false,
                 bool xsts_token_verified=false,
                 unsigned int xbox_xerr=0,
                 int xbox_http_status=0,int xsts_http_status=0,
                 bool cloud_login_verified=false,int cloud_http_status=0,
                 unsigned int cloud_regions=0);
    CRITICAL_SECTION lock_;
    HANDLE thread_;
    volatile LONG cancel_requested_;
    volatile LONG forget_requested_;
    int cache_status_;
    bool cache_available_;
    bool cache_resumed_;
    Snapshot current_;
    char client_id_[64];
};
