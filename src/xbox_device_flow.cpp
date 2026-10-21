#include "xbox_device_flow.h"
#include "xbox_live_auth.h"
#include "json_fields.h"
#include "http_wire.h"
#include <string.h>
#include <stdlib.h>

namespace xbox_device_flow {
namespace {
bool public_guid(const char* id) {
    if(!id || strlen(id)!=36)return false;
    for(int i=0;i<36;++i) {
        const char c=id[i];
        if(i==8||i==13||i==18||i==23) {
            if(c!='-')return false;
        } else if(!((c>='0'&&c<='9') ||
                    (c>='a'&&c<='f') ||
                    (c>='A'&&c<='F'))) return false;
    }
    return true;
}
bool scope_valid(const char* s) {
    if(!s)return false;
    const size_t n=strlen(s);
    if(!n||n>255)return false;
    for(size_t i=0;i<n;++i) {
        const unsigned char c=(unsigned char)s[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='.'||c=='_'||c==' '||
             c=='-'||c==':'))return false;
    }
    return true;
}
bool copy(char* dest,size_t cap,const char* value) {
    if(!dest||!value || strlen(value)>=cap)return false;
    strcpy_s(dest,cap,value);
    return true;
}
bool due(unsigned long now,unsigned long deadline) {
    return (long)(now-deadline)>=0; // safe for <24.8d time intervals
}
bool add(char* out,size_t cap,size_t* pos,const char* string) {
    const size_t n=strlen(string);
    if(!out||!pos || *pos>=cap || n>=cap-*pos)return false;
    memcpy(out+*pos,string,n);*pos+=n;
    out[*pos]=0;
    return true;
}
}
Flow::Flow():transport_(NULL),state_(MISSING_CONFIG),next_poll_ms_(0),
             expires_at_ms_(0),token_expires_s_(0),poll_attempts_(0),
             http_status_(0) {
    memset(&info_,0,sizeof(info_));
    memset(client_id_,0,sizeof(client_id_));
    memset(scopes_,0,sizeof(scopes_));
    memset(access_token_,0,sizeof(access_token_));
    memset(refresh_token_,0,sizeof(refresh_token_));
    memset(granted_scopes_,0,sizeof(granted_scopes_));
    memset(oauth_error_,0,sizeof(oauth_error_));
}
Flow::~Flow() {cancel();}
void Flow::cancel() {
    oauth_device::clear(&info_);
    secure_http::secure_zero(access_token_,sizeof(access_token_));
    secure_http::secure_zero(refresh_token_,sizeof(refresh_token_));
    secure_http::secure_zero(granted_scopes_,sizeof(granted_scopes_));
    next_poll_ms_=0;expires_at_ms_=0;token_expires_s_=0;
    poll_attempts_=0;
    http_status_=0;
    state_=transport_?READY:MISSING_CONFIG;
}
bool Flow::configure(const char* id,const char* scope,
                     const secure_http::Transport* tls) {
    cancel();
    secure_http::secure_zero(client_id_,sizeof(client_id_));
    secure_http::secure_zero(scopes_,sizeof(scopes_));
    secure_http::secure_zero(oauth_error_,sizeof(oauth_error_));
    transport_=NULL;
    if(!public_guid(id)||!scope_valid(scope)) {
        state_=MISSING_CONFIG;
        return false;
    }
    if(!copy(client_id_,sizeof(client_id_),id) ||
       !copy(scopes_,sizeof(scopes_),scope))return false;
    transport_=tls;
    state_=(tls&&tls->configured())?READY:TRANSPORT_REQUIRED;
    return true;
}
void Flow::fail() {
    oauth_device::clear(&info_);
    secure_http::secure_zero(access_token_,sizeof(access_token_));
    secure_http::secure_zero(refresh_token_,sizeof(refresh_token_));
    secure_http::secure_zero(granted_scopes_,sizeof(granted_scopes_));
    state_=FAILED;
}
bool Flow::post(const char* path,const char* form,
                secure_http::Response* result) {
    return transport_ && transport_->configured() &&
           transport_->post_form("login.microsoftonline.com",path,
                                 form,strlen(form),result);
}
bool Flow::start(unsigned long now) {
    if(!client_id_[0]) {
        state_=MISSING_CONFIG;return false;
    }
    if(!transport_ || !transport_->configured()) {
        state_=TRANSPORT_REQUIRED;return false;
    }
    cancel();
    secure_http::secure_zero(oauth_error_,sizeof(oauth_error_));
    state_=REQUESTING;
    char form[1024];
    if(!oauth_device::make_device_form(client_id_,scopes_,
                                      form,sizeof(form))) {
        fail();return false;
    }
    // This call MUST be driven from a background worker when a real HTTPS
    // backend is installed; never block Direct3D rendering for network.
    secure_http::Response* response=
        static_cast<secure_http::Response*>(malloc(sizeof(secure_http::Response)));
    if(!response) {
        secure_http::secure_zero(form,sizeof(form));
        fail();return false;
    }
    const bool sent=post("/consumers/oauth2/v2.0/devicecode",form,response);
    secure_http::secure_zero(form,sizeof(form));
    const bool parsed=sent && response->http_status==200 &&
        oauth_device::read_device_response(response->body,response->length,
                                          &info_);
    if(sent)http_status_=response->http_status;
    secure_http::secure_zero(response,sizeof(*response));
    free(response);
    if(!parsed) {fail();return false;}
    expires_at_ms_=now+info_.expires_in*1000ul;
    next_poll_ms_=now+info_.interval*1000ul;
    state_=WAITING_USER;
    return true;
}
void Flow::code_received_at(unsigned long now) {
    if(state_!=WAITING_USER)return;
    expires_at_ms_=now+info_.expires_in*1000ul;
    // Give a person at the keyboard at least 30 seconds to open Microsoft's
    // site before the first token check. Subsequent polls honor the server
    // interval and slow_down replies. Never extend the SERVER expiry.
    const unsigned int first_poll_seconds =
        info_.interval>30u ? info_.interval : 30u;
    next_poll_ms_=now+first_poll_seconds*1000ul;
}
unsigned int Flow::next_poll_seconds(unsigned long now) const {
    if(state_!=WAITING_USER || due(now,next_poll_ms_))return 0;
    return (unsigned int)((next_poll_ms_-now+999ul)/1000ul);
}
void Flow::poll_completed_at(unsigned long now) {
    // Called only after an actual completed blocking token request.
    // The interval always starts after the HTTPS response, avoiding
    // accidental busy polling on slow TLS/network connections.
    if(state_==WAITING_USER)
        next_poll_ms_=now+info_.interval*1000ul;
}
unsigned int Flow::remaining_seconds(unsigned long now) const {
    if(state_!=WAITING_USER || due(now,expires_at_ms_))return 0;
    return (unsigned int)((expires_at_ms_-now+999ul)/1000ul);
}
bool Flow::handle_token_reply(const secure_http::Response& result,
                              unsigned long now) {
    if(result.http_status==200) {
        char token_type[32];
        unsigned int lifetime=0;
        // Reject malformed/partial tokens; tokens must never be logged.
        if(!json_fields::string(result.body,result.length,"access_token",
                                access_token_,sizeof(access_token_)) ||
           !json_fields::string(result.body,result.length,"token_type",
                                token_type,sizeof(token_type)) ||
           strcmp(token_type,"Bearer")!=0 ||
           !json_fields::uint_value(result.body,result.length,"expires_in",
                                     &lifetime) || lifetime==0 ||
           lifetime>86400 || !access_token_[0]) {
            fail();return false;
        }
        // Refresh tokens can be absent depending on granted scope.
        json_fields::string(result.body,result.length,"refresh_token",
                            refresh_token_,sizeof(refresh_token_));
        token_expires_s_=lifetime;
        // Explicit scope from the actual HTTPS token response is required
        // to mark an Xbox permission as verified. Requested scopes are not
        // proof of consent, and the field may be absent.
        secure_http::secure_zero(granted_scopes_,sizeof(granted_scopes_));
        json_fields::string(result.body,result.length,"scope",
                            granted_scopes_,sizeof(granted_scopes_));
        oauth_device::clear(&info_);
        state_=AUTHORIZED;
        return true;
    }
    char error[96];
    if(!json_fields::string(result.body,result.length,"error",
                            error,sizeof(error))) {
        strcpy_s(oauth_error_,sizeof(oauth_error_),"HTTP_ERROR_NO_VALID_CODE");
        fail();return false;
    }
    // Only expose the documented ASCII OAuth *error name*, never the
    // error_description (which may contain personally identifying text).
    bool safe_name=true;
    for(size_t i=0;error[i];++i) {
        const unsigned char c=(unsigned char)error[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
             (c>='0'&&c<='9')||c=='_'||c=='-')) {
            safe_name=false;break;
        }
    }
    if(safe_name)strcpy_s(oauth_error_,sizeof(oauth_error_),error);
    else strcpy_s(oauth_error_,sizeof(oauth_error_),"INVALID_SERVER_ERROR_NAME");
    if(strcmp(error,"authorization_pending")==0) {
        next_poll_ms_=now+info_.interval*1000ul;
        return true;
    }
    if(strcmp(error,"slow_down")==0) {
        if(info_.interval<=115)info_.interval+=5;
        else info_.interval=120;
        next_poll_ms_=now+info_.interval*1000ul;
        return true;
    }
    if(strcmp(error,"expired_token")==0) {
        oauth_device::clear(&info_);
        state_=EXPIRED;return false;
    }
    if(strcmp(error,"authorization_declined")==0 ||
       strcmp(error,"access_denied")==0) {
        oauth_device::clear(&info_);
        state_=DECLINED;return false;
    }
    fail();
    return false;
}
bool Flow::poll(unsigned long now) {
    if(state_!=WAITING_USER)return false;
    if(due(now,expires_at_ms_)) {
        oauth_device::clear(&info_);
        state_=EXPIRED;
        return false;
    }
    if(!due(now,next_poll_ms_))return true;
    ++poll_attempts_;
    char client[256]={0},code[4096]={0},form[4608]={0};
    bool ok=http_wire::form_escape(client_id_,client,sizeof(client)) &&
            http_wire::form_escape(info_.device_code,code,sizeof(code));
    size_t at=0;
    if(ok) {
        ok=add(form,sizeof(form),&at,
               "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code") &&
           add(form,sizeof(form),&at,"&client_id=") &&
           add(form,sizeof(form),&at,client) &&
           add(form,sizeof(form),&at,"&device_code=") &&
           add(form,sizeof(form),&at,code);
    }
    secure_http::secure_zero(code,sizeof(code));
    if(!ok) {secure_http::secure_zero(form,sizeof(form));fail();return false;}
    secure_http::Response* reply=
        static_cast<secure_http::Response*>(malloc(sizeof(secure_http::Response)));
    if(!reply) {
        secure_http::secure_zero(form,sizeof(form));
        fail();return false;
    }
    const bool sent=post("/consumers/oauth2/v2.0/token",form,reply);
    secure_http::secure_zero(form,sizeof(form));
    if(sent)http_status_=reply->http_status;
    const bool accepted=sent?handle_token_reply(*reply,now):false;
    secure_http::secure_zero(reply,sizeof(*reply));
    free(reply);
    if(!sent) {
        // Transient HTTPS errors during polling must not erase a valid
        // device code. Never accept an unverified response; retry later.
        next_poll_ms_=now+info_.interval*1000ul;
        return true;
    }
    return accepted;
}
bool Flow::xbox_scope_granted() const {
    return state_==AUTHORIZED &&
           (xbox_live_auth::has_scope(granted_scopes_,"XboxLive.signin") ||
            xbox_live_auth::has_scope(granted_scopes_,"xboxlive.signin"));
}
const char* Flow::xbox_token_for_worker() const {
    return xbox_scope_granted() && access_token_[0] ? access_token_ : NULL;
}
const char* Flow::status_text() const {
    switch(state_) {
        case MISSING_CONFIG:return "REGISTERED CLIENT ID REQUIRED";
        case TRANSPORT_REQUIRED:return "VERIFIED TLS BACKEND REQUIRED";
        case READY:return "READY TO REQUEST DEVICE CODE";
        case REQUESTING:return "REQUESTING DEVICE CODE";
        case WAITING_USER:return oauth_error_[0] ? oauth_error_ :
                                    "ENTER CODE ON YOUR PHONE";
        case AUTHORIZED:return "MSA TOKEN RECEIVED - XBL STILL PENDING";
        case EXPIRED:return "DEVICE CODE EXPIRED";
        case DECLINED:return "SIGN-IN DECLINED";
        case FAILED:return oauth_error_[0] ? oauth_error_ : "AUTH REQUEST FAILED";
        default:return "UNKNOWN";
    }
}
}

namespace xbox_device_flow {
namespace {
struct OfflineMock {
    unsigned int step;
    bool verified;
};
bool simulated_https(void* user,const char* host,const char* path,
                     const char* form,size_t bytes,
                     secure_http::Response* result) {
    OfflineMock* mock=static_cast<OfflineMock*>(user);
    if(!mock||!host||!path||!form||!bytes||!result)return false;
    if(strcmp(host,"login.microsoftonline.com")!=0)return false;
    const char* json=NULL;
    if(mock->step==0 &&
       strcmp(path,"/consumers/oauth2/v2.0/devicecode")==0) {
        json="{\"verification_uri\":\"https://www.microsoft.com/link\","
             "\"user_code\":\"SYNTHETIC\",\"device_code\":\"fake_device\","
             "\"expires_in\":900,\"interval\":5}";
        result->http_status=200;
    } else if(mock->step==1 &&
              strcmp(path,"/consumers/oauth2/v2.0/token")==0) {
        json="{\"error\":\"authorization_pending\"}";
        result->http_status=400;
    } else if(mock->step==2) {
        json="{\"error\":\"slow_down\"}";
        result->http_status=400;
    } else if(mock->step==3) {
        json="{\"token_type\":\"Bearer\",\"access_token\":\"synthetic_access\","
             "\"refresh_token\":\"synthetic_refresh\",\"expires_in\":3600}";
        result->http_status=200;
    } else return false;
    const size_t length=strlen(json);
    if(length>=secure_http::RESPONSE_CAP)return false;
    memcpy(result->body,json,length);
    result->length=length;
    result->peer_certificate_verified=mock->verified;
    result->hostname_verified=mock->verified;
    ++mock->step;
    return true;
}
}
bool offline_selftest() {
    // No socket creation and no Microsoft request: entirely synthetic.
    static Flow flow; // avoid >20 KiB stack usage on the Xbox's main thread
    secure_http::Transport https;
    const char id[]="00000000-0000-4000-8000-000000000001";
    if(!flow.configure(id,"openid profile offline_access",&https) ||
       flow.state()!=TRANSPORT_REQUIRED || flow.start(1000) ||
       flow.state()!=TRANSPORT_REQUIRED)
        return false;
    OfflineMock mock={0,false};
    https.attach(simulated_https,&mock);
    if(!flow.configure(id,"openid profile offline_access",&https))return false;
    // Unverified TLS responses must never produce a device code.
    if(flow.start(1000) || flow.state()!=FAILED)return false;
    mock.step=0;
    mock.verified=true;
    if(!flow.configure(id,"openid profile offline_access",&https) ||
       !flow.start(1000) || flow.state()!=WAITING_USER ||
       strcmp(flow.user_code(),"SYNTHETIC")!=0 ||
       flow.remaining_seconds(1000)!=900)return false;
    // Simulate a 3-second HTTPS device-code request. Even after a valid
    // code arrives, keep it displayed before the first poll at 30 seconds.
    flow.code_received_at(4000);
    if(flow.remaining_seconds(4000)!=900 ||
       !flow.poll(4001) || !flow.poll(9000) ||
       mock.step!=1 || flow.state()!=WAITING_USER)return false;
    if(!flow.poll(34000) || flow.state()!=WAITING_USER ||
       flow.poll_attempts()!=1 ||
       strcmp(flow.status_text(),"authorization_pending")!=0)return false;
    flow.poll_completed_at(37000); // Slow HTTPS roundtrip must postpone next poll
    if(flow.next_poll_seconds(37000)!=5 || !flow.poll(39000) ||
       flow.poll_attempts()!=1)return false;
    if(!flow.poll(42000) || flow.interval_seconds()!=10 ||
       flow.poll_attempts()!=2 ||
       strcmp(flow.status_text(),"slow_down")!=0)return false;
    flow.poll_completed_at(44000);
    if(flow.next_poll_seconds(44000)!=10 ||
       !flow.poll(50000) || flow.poll_attempts()!=2)return false;
    if(!flow.poll(54000) || flow.state()!=AUTHORIZED ||
       !flow.is_authorized() || flow.xbox_scope_granted())return false;
    flow.cancel();
    return flow.state()==READY && !flow.is_authorized();
}
}
