#include "xbox_account.h"
#include "xbox_device_flow.h"
#include "xbox_live_auth.h"
#include "xbox_live_exchange.h"
#include "secure_http.h"
#include "xboxtls_backend.h"
#include "xcloud_session.h"
#include "login_cache.h"
#include "login_cache_platform.h"
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
struct WorkerCacheContext {
    XboxAccount* account;
    login_cache::Store* store;
};
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
XboxAccount::XboxAccount():thread_(NULL),cancel_requested_(0),forget_requested_(0),
    cache_status_(login_cache::MISSING),cache_available_(false),cache_resumed_(false) {
    InitializeCriticalSection(&lock_);
    memset(&current_,0,sizeof(current_));
    current_.state=IDLE;
    memset(client_id_,0,sizeof(client_id_));
}
XboxAccount::~XboxAccount() {
    cancel();
    if(thread_){WaitForSingleObject(thread_,INFINITE);CloseHandle(thread_);}
    if(InterlockedCompareExchange(&forget_requested_,0,0)!=0)erase_saved_login();
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
    memset(&current_,0,sizeof(current_));
    current_.state=status;
    current_.cache_status=cache_status_;
    current_.cache_available=cache_available_;
    current_.cache_resumed=cache_resumed_;
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
    if(InterlockedCompareExchange(&forget_requested_,0,0)!=0 && thread_ &&
       WaitForSingleObject(thread_,0)==WAIT_OBJECT_0) {
        CloseHandle(thread_);thread_=NULL;erase_saved_login();
    }
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
    if(InterlockedCompareExchange(&forget_requested_,0,0)!=0)return false;
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
    if(self) {
        if(InterlockedCompareExchange(&self->forget_requested_,0,0)==0)self->perform();
        if(InterlockedCompareExchange(&self->forget_requested_,0,0)!=0)self->erase_saved_login();
    }
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
void XboxAccount::publish_cache(int status,bool available,bool resumed) {
    EnterCriticalSection(&lock_);
    cache_status_=status;cache_available_=available;cache_resumed_=resumed;
    current_.cache_status=status;current_.cache_available=available;current_.cache_resumed=resumed;
    LeaveCriticalSection(&lock_);
}
bool XboxAccount::persist_login(void* context,const char* client,const char* requested,
                                const char* granted,const char* refresh) {
    WorkerCacheContext* cache=static_cast<WorkerCacheContext*>(context);
    XboxAccount* self=cache->account;
    if(InterlockedCompareExchange(&self->forget_requested_,0,0)!=0)return false;
    const login_cache::Status saved=cache->store->save(client,requested,granted,refresh);
    const Snapshot before=self->snapshot();
    self->publish_cache((int)saved,saved==login_cache::OK || before.cache_available,before.cache_resumed);
    return saved==login_cache::OK;
}
bool XboxAccount::erase_saved_login() {
    LoginCachePlatform backend;
    const bool erased=backend.erase();
    publish_cache(erased?login_cache::MISSING:login_cache::IO_ERROR,!erased,false);
    if(erased)InterlockedExchange(&forget_requested_,0);
    publish(erased?USER_CANCELLED:LOGIN_FAILED,
        erased?"SAVED LOGIN REMOVED":"SAVED LOGIN REMOVE FAILED",NULL,NULL,0,0,0,0);
    return erased;
}
bool XboxAccount::forget_saved_login() {
    InterlockedExchange(&forget_requested_,1);
    cancel();
    if(thread_ && WaitForSingleObject(thread_,0)==WAIT_TIMEOUT)return true;
    if(thread_){CloseHandle(thread_);thread_=NULL;}
    return erase_saved_login();
}
void XboxAccount::perform() {
    XboxTlsBackend backend;
    secure_http::Transport transport;
    transport.attach(&XboxTlsBackend::post_form,&backend);
    // This Flow owns device/token secrets. It is stack-local to the
    // worker and its destructor wipes everything on every exit path.
    xbox_device_flow::Flow flow;
    LoginCachePlatform cache_backend;
    login_cache::Store cache(cache_backend);
    WorkerCacheContext cache_context={this,&cache};
    if(!flow.configure(client_id_,xbox_live_auth::configured_scopes(),&transport)) {
        publish(LOGIN_FAILED,"INVALID CLIENT CONFIG",
                NULL,NULL,0,0,0,0);
        return;
    }
    if(InterlockedCompareExchange(&cancel_requested_,0,0)!=0) {
        publish(USER_CANCELLED,"CANCELLED",NULL,NULL,0,0,0,0);
        return;
    }
    flow.set_persistence(&XboxAccount::persist_login,&cache_context);
    bool resumed=false;
    login_cache::SavedLogin* saved=new(std::nothrow) login_cache::SavedLogin;
    const login_cache::Status loaded=saved ?
        cache.load(client_id_,xbox_live_auth::configured_scopes(),saved):login_cache::IO_ERROR;
    publish_cache((int)loaded,loaded==login_cache::OK,false);
    if(loaded==login_cache::OK && !worker_cancelled(this)) {
        publish(STARTING,"RESTORING SAVED LOGIN",NULL,NULL,0,0,0,0);
        resumed=flow.resume_from_refresh(saved->refresh,saved->granted,GetTickCount());
    }
    delete saved; // Destructor erases the decrypted cache immediately.
    if(worker_cancelled(this)) {
        flow.cancel();publish(USER_CANCELLED,"CANCELLED",NULL,NULL,0,0,0,0);return;
    }
    if(loaded==login_cache::OK && !resumed) {
        if(flow.reauthentication_required()) {
            const bool erased=cache.erase();
            publish_cache(erased?login_cache::MISSING:login_cache::IO_ERROR,!erased,false);
            // A verified authorization error requires interactive sign-in.
            flow.configure(client_id_,xbox_live_auth::configured_scopes(),&transport);
        } else {
            XboxTlsBackend::Diagnostic diag;backend.get_diagnostic(&diag);
            publish(LOGIN_FAILED,flow.status_text(),NULL,NULL,diag.tls_error,
                    diag.wsa_error,flow.last_http_status(),0);
            return; // Preserve the saved login on transient network/service errors.
        }
    }
    if(resumed) {
        const Snapshot cache_state=snapshot();
        publish_cache(cache_state.cache_status,cache_state.cache_available,true);
    }
    if(!resumed && !flow.start(GetTickCount())) {
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
                if(authorized) {
                    perform_session(flow,backend,credentials);
                    credentials.clear();
                    publish_credentials(0);
                }
                if(flow.reauthentication_required()) {
                    const bool erased=cache.erase();
                    publish_cache(erased?login_cache::MISSING:login_cache::IO_ERROR,!erased,false);
                }
                flow.cancel();
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
void XboxAccount::publish_session(const xcloud_session::Session& session,
                                  bool complete,const char* override_error) {
    const xcloud_session::Snapshot& state=session.snapshot();
    EnterCriticalSection(&lock_);
    current_.session_stage=(unsigned int)state.stage;
    current_.session_http_status=state.http_status;
    current_.session_cleanup_http=state.cleanup_http;
    current_.session_wait_seconds=state.queue_seconds;
    current_.session_created=state.session_created;
    current_.session_closed=state.session_created && !state.cleanup_pending;
    current_.session_test_complete=complete;
    current_.session_cleanup_failed=complete && state.cleanup_pending;
    bounded_copy(current_.session_server_state,sizeof(current_.session_server_state),state.server_state);
    bounded_copy(current_.session_error,sizeof(current_.session_error),
                 override_error?override_error:state.error_code);
    bounded_copy(current_.stage,sizeof(current_.stage),xcloud_session::stage_text(state.stage));
    LeaveCriticalSection(&lock_);
    // Only numeric results and a restricted error code are logged.
    char log[240];
    sprintf_s(log,sizeof(log),"[XCLOUD360] SESSION stage=%u HTTP=%d cleanup=%d created=%d complete=%d code=%.63s\n",
        (unsigned int)state.stage,state.http_status,state.cleanup_http,
        state.session_created?1:0,complete?1:0,override_error?override_error:state.error_code);
    OutputDebugStringA(log);
}
void XboxAccount::perform_session(xbox_device_flow::Flow& flow,
                                  XboxTlsBackend& backend,
                                  xbox_live_exchange::CloudCredentials& credentials) {
    xcloud_session::Session session;
    session.attach(&XboxTlsBackend::cloud_request,&backend);
    const unsigned int begin=worker_clock(this);
    const char* error=NULL;
    publish_credentials(credentials.remaining_seconds(begin));
    if(worker_cancelled(this))return;
    const bool created=session.start(credentials.host_for_worker(begin),
                                     credentials.token_for_worker(begin));
    publish_session(session,false);
    bool provisioned=false;
    while(created && !worker_cancelled(this)) {
        const xcloud_session::Stage stage=session.snapshot().stage;
        if(stage==xcloud_session::FAILED)break;
        if(stage==xcloud_session::PROVISIONED) {provisioned=true;break;}
        unsigned int now=worker_clock(this);
        if(now-begin>=300000u) {error="SESSION_TEST_TIMEOUT";break;}
        // Renew before expiration, never replay a rejected old credential in
        // a tight loop. The Session keeps its original regional hostname.
        if(credentials.remaining_seconds(now)<=60u) {
            if(!flow.refresh_for_worker(now) || worker_cancelled(this)) {
                error="MICROSOFT_REFRESH_FAILED";break;
            }
            xbox_live_exchange::CloudCredentials replacement;
            xbox_live_exchange::WorkerContext worker={&replacement,
                &XboxAccount::worker_clock,&XboxAccount::worker_cancelled,this};
            xbox_live_exchange::Result renewed;
            if(!xbox_live_exchange::exchange(flow.xbox_token_for_worker(),
                flow.xbox_scope_granted(),&XboxTlsBackend::post_xbox_json,
                &backend,&renewed,&worker)) {error="XCLOUD_REFRESH_FAILED";break;}
            now=worker_clock(this);
            const unsigned int lifetime=replacement.remaining_seconds(now);
            if(lifetime<=60u) {
                error="XCLOUD_CREDENTIAL_TOO_SHORT";break;
            }
            char uri[160];
            sprintf_s(uri,sizeof(uri),"https://%s",replacement.host_for_worker(now));
            if(!credentials.retain(replacement.token_for_worker(now),uri,lifetime,now)) {
                error="XCLOUD_REFRESH_INVALID";break;
            }
        }
        publish_credentials(credentials.remaining_seconds(now));
        if(stage==xcloud_session::READY_TO_CONNECT) {
            char* transfer=(char*)malloc(12288);
            if(!transfer) {error="TRANSFER_NO_MEMORY";break;}
            memset(transfer,0,12288);
            const bool fetched=flow.request_transfer_token(transfer,12288);
            const bool cancelled=worker_cancelled(this);
            bool connected=false;
            if(fetched && !cancelled)
                connected=session.connect(credentials.token_for_worker(worker_clock(this)),transfer);
            secure_http::secure_zero(transfer,12288);free(transfer);
            if(cancelled)break;
            if(!fetched) {error="TRANSFER_TOKEN_FAILED";break;}
            publish_session(session,false);
            if(!connected)break;
        }
        // All pacing occurs on the worker, preserving responsive rendering
        // and B cancellation even while the service reports a queue.
        const unsigned int pause=worker_clock(this);
        while(!worker_cancelled(this) && worker_clock(this)-pause<3000u)Sleep(50);
        if(worker_cancelled(this))break;
        if(!session.poll(credentials.token_for_worker(worker_clock(this)))) {
            publish_session(session,false);break;
        }
        publish_session(session,false);
    }
    if(worker_cancelled(this))error="SESSION_CANCELLED";
    // Current build proves the control plane, then releases the game VM.
    // A media-ready implementation must replace this terminal checkpoint.
    for(unsigned int attempt=0;attempt<3u && session.needs_cleanup();++attempt) {
        const char* token=credentials.token_for_worker(worker_clock(this));
        if(!token) {if(!error)error="SESSION_CLEANUP_CREDENTIAL_EXPIRED";break;}
        if(session.stop(token))break;
        Sleep(200);
    }
    if(!error && !provisioned && !session.snapshot().error_code[0])
        error="SESSION_NOT_PROVISIONED";
    publish_session(session,true,error);
}
