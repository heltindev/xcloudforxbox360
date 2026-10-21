#include "xbox_account.h"
#include "xbox_device_flow.h"
#include "xbox_live_auth.h"
#include "xbox_live_exchange.h"
#include "secure_http.h"
#include "xboxtls_backend.h"
#include <string.h>

namespace {
bool valid_id(const char* id) {
    if(!id || strlen(id)!=36)return false;
    for(int i=0;i<36;++i) {
        const char c=id[i];
        if(i==8||i==13||i==18||i==23) {
            if(c!='-')return false;
        } else if(!((c>='0'&&c<='9') ||
                    (c>='a'&&c<='f') ||
                    (c>='A'&&c<='F')))return false;
    }
    return true;
}
void bounded_copy(char* dst,size_t cap,const char* src) {
    if(!src)src="";
    strncpy_s(dst,cap,src,_TRUNCATE);
}
}
XboxAccount::XboxAccount():thread_(NULL),cancel_requested_(0) {
    InitializeCriticalSection(&lock_);
    memset(&current_,0,sizeof(current_));
    current_.state=IDLE;
    memset(client_id_,0,sizeof(client_id_));
}
XboxAccount::~XboxAccount() {
    cancel();
    if(thread_){WaitForSingleObject(thread_,INFINITE);CloseHandle(thread_);}
    secure_http::secure_zero(client_id_,sizeof(client_id_));
    DeleteCriticalSection(&lock_);
}
void XboxAccount::publish(Status status,const char* stage,
                          const char* url,const char* code,
                          int tls,int wsa,int http,unsigned int remaining,
                          unsigned int attempts,unsigned int next_poll,
                          bool xbox_scope_verified,
                          bool xbox_user_token_verified,
                          bool xsts_token_verified,
                          unsigned int xbox_xerr,
                          int xbox_http_status,int xsts_http_status,
                          bool cloud_login_verified,int cloud_http_status,
                          unsigned int cloud_regions) {
    EnterCriticalSection(&lock_);
    current_.state=status;
    current_.cloud_credentials_ready=false;
    current_.cloud_credentials_seconds=0;
    bounded_copy(current_.stage,sizeof(current_.stage),stage);
    bounded_copy(current_.verification_url,
                 sizeof(current_.verification_url),url);
    bounded_copy(current_.user_code,sizeof(current_.user_code),code);
    current_.tls_error=tls;
    current_.wsa_error=wsa;
    current_.http_status=http;
    current_.seconds_left=remaining;
    current_.poll_attempts=attempts;
    current_.next_poll_seconds=next_poll;
    current_.xbox_scope_verified=xbox_scope_verified;
    current_.xbox_user_token_verified=xbox_user_token_verified;
    current_.xsts_token_verified=xsts_token_verified;
    current_.xbox_xerr=xbox_xerr;
    current_.xbox_http_status=xbox_http_status;
    current_.xsts_http_status=xsts_http_status;
    current_.cloud_login_verified=cloud_login_verified;
    current_.cloud_http_status=cloud_http_status;
    current_.cloud_regions=cloud_regions;
    LeaveCriticalSection(&lock_);
}
XboxAccount::Snapshot XboxAccount::snapshot() {
    Snapshot local;
    EnterCriticalSection(&lock_);
    local=current_;
    LeaveCriticalSection(&lock_);
    return local;
}
void XboxAccount::cancel() {
    InterlockedExchange(&cancel_requested_,1);
}
bool XboxAccount::start(const char* id) {
    if(!valid_id(id)) {
        publish(NOT_CONFIGURED,"REGISTERED CLIENT ID REQUIRED",
                NULL,NULL,0,0,0,0);
        return false;
    }
    if(thread_) {
        if(WaitForSingleObject(thread_,0)==WAIT_TIMEOUT)return false;
        CloseHandle(thread_);thread_=NULL;
    }
    secure_http::secure_zero(client_id_,sizeof(client_id_));
    bounded_copy(client_id_,sizeof(client_id_),id);
    InterlockedExchange(&cancel_requested_,0);
    publish(STARTING,"INIT TLS AND REQUEST DEVICE CODE",
            NULL,NULL,0,0,0,0);
    thread_=CreateThread(NULL,0,&XboxAccount::worker_main,this,0,NULL);
    if(!thread_) {
        publish(LOGIN_FAILED,"WORKER THREAD ERROR",
                NULL,NULL,0,0,0,0);
        return false;
    }
    return true;
}
DWORD WINAPI XboxAccount::worker_main(LPVOID arg) {
    XboxAccount* self=static_cast<XboxAccount*>(arg);
    if(self)self->perform();
    return 0;
}
unsigned int XboxAccount::worker_clock(void*) {return (unsigned int)GetTickCount();}
bool XboxAccount::worker_cancelled(void* context) {
    XboxAccount* self=static_cast<XboxAccount*>(context);
    return InterlockedCompareExchange(&self->cancel_requested_,0,0)!=0;
}
void XboxAccount::publish_credentials(unsigned int seconds) {
    EnterCriticalSection(&lock_);
    current_.cloud_credentials_ready=seconds!=0;
    current_.cloud_credentials_seconds=seconds;
    LeaveCriticalSection(&lock_);
}
void XboxAccount::perform() {
    XboxTlsBackend backend;
    secure_http::Transport transport;
    transport.attach(&XboxTlsBackend::post_form,&backend);
    // This Flow owns device/token secrets. It is stack-local to the
    // worker and its destructor wipes everything on every exit path.
    xbox_device_flow::Flow flow;
    if(!flow.configure(client_id_,xbox_live_auth::configured_scopes(),&transport)) {
        publish(LOGIN_FAILED,"INVALID CLIENT CONFIG",
                NULL,NULL,0,0,0,0);
        return;
    }
    if(InterlockedCompareExchange(&cancel_requested_,0,0)!=0) {
        publish(USER_CANCELLED,"CANCELLED",NULL,NULL,0,0,0,0);
        return;
    }
    if(!flow.start(GetTickCount())) {
        XboxTlsBackend::Diagnostic diag;
        backend.get_diagnostic(&diag);
        publish(LOGIN_FAILED,diag.stage,NULL,NULL,
                diag.tls_error,diag.wsa_error,
                flow.last_http_status(),0);
        return;
    }
    // The network request can take many seconds. Start the poll interval
    // when the *device code arrives*, not before the HTTPS handshake.
    flow.code_received_at(GetTickCount());
    while(InterlockedCompareExchange(&cancel_requested_,0,0)==0) {
        const unsigned long now=GetTickCount();
        const xbox_device_flow::State state=flow.state();
        if(state==xbox_device_flow::AUTHORIZED) {
            const bool xbox_scope=flow.xbox_scope_granted();
            if(xbox_scope &&
               InterlockedCompareExchange(&cancel_requested_,0,0)==0) {
                publish(XBOX_EXCHANGING,
                        "XBOX USER / XSTS TLS TOKEN EXCHANGE",
                        NULL,NULL,0,0,flow.last_http_status(),0,
                        0,0,true);
                xbox_live_exchange::CloudCredentials credentials;
                xbox_live_exchange::WorkerContext worker={
                    &credentials,&XboxAccount::worker_clock,
                    &XboxAccount::worker_cancelled,this};
                xbox_live_exchange::Result result;
                const bool authorized=xbox_live_exchange::exchange(
                    flow.xbox_token_for_worker(),true,
                    &XboxTlsBackend::post_xbox_json,&backend,&result,&worker);
                // Each operation verifies its certificate chain/hostname;
                // intermediate tokens are wiped; gsToken has one worker owner.
                if(InterlockedCompareExchange(&cancel_requested_,0,0)!=0) {
                    flow.cancel();
                    publish(USER_CANCELLED,"CANCELLED",
                            NULL,NULL,0,0,0,0);
                    return;
                }
                XboxTlsBackend::Diagnostic diag;
                backend.get_diagnostic(&diag);
                publish(ACCOUNT_AUTHORIZED,
                    xbox_live_exchange::stage_text(result.stage),
                    NULL,NULL,diag.tls_error,diag.wsa_error,
                    flow.last_http_status(),0,0,0,true,
                    result.xbox_user_received,result.xsts_received,
                    result.xerr,result.user_http,result.xsts_http,
                    result.cloud_login_received,result.cloud_http,result.cloud_regions);
                flow.cancel(); // Erase MSA/device secrets after the exchange.
                if(authorized) {
                    // Keep gsToken in the worker for future session integration.
                    // This checkpoint does not allocate a remote game VM.
                    while(!worker_cancelled(this)) {
                        const unsigned int left=credentials.remaining_seconds(worker_clock(this));
                        publish_credentials(left);
                        if(!left) {
                            publish(LOGIN_FAILED,"XCLOUD LOGIN EXPIRED",
                                    NULL,NULL,0,0,0,0);
                            return;
                        }
                        Sleep(100);
                    }
                    credentials.clear();
                    publish(USER_CANCELLED,"CANCELLED",NULL,NULL,0,0,0,0);
                }
            } else {
                publish(ACCOUNT_AUTHORIZED,
                    "MSA OK / XBOX SCOPE NOT CONFIRMED",
                    NULL,NULL,0,0,flow.last_http_status(),0,
                    0,0,false);
            }
            return; // Flow destructor erases the MSA access token.
        }
        if(state!=xbox_device_flow::WAITING_USER) {
            XboxTlsBackend::Diagnostic diag;
            backend.get_diagnostic(&diag);
            publish(LOGIN_FAILED,flow.status_text(),NULL,NULL,
                    diag.tls_error,diag.wsa_error,
                    flow.last_http_status(),0);
            return;
        }
        publish(WAITING_FOR_USER,flow.status_text(),
                flow.verification_url(),flow.user_code(),
                0,0,flow.last_http_status(),
                flow.remaining_seconds(now),flow.poll_attempts(),
                flow.next_poll_seconds(now));
        // If an actual poll happens, start the NEXT minimum interval
        // after the completed HTTP operation, not before it. This keeps
        // authorization_pending / slow_down on the same visible code.
        const unsigned int before=flow.poll_attempts();
        flow.poll(now);
        if(flow.poll_attempts()!=before) {
            const unsigned long end=GetTickCount();
            flow.poll_completed_at(end);
            if(flow.state()==xbox_device_flow::WAITING_USER)
                publish(WAITING_FOR_USER,flow.status_text(),
                        flow.verification_url(),flow.user_code(),
                        0,0,flow.last_http_status(),
                        flow.remaining_seconds(end),flow.poll_attempts(),
                        flow.next_poll_seconds(end));
        }
        Sleep(100);
    }
    flow.cancel();
    publish(USER_CANCELLED,"CANCELLED",NULL,NULL,0,0,0,0);
}
