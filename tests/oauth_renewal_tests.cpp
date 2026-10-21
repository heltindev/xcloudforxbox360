#include "xbox_device_flow.h"
#include <stdio.h>
#include <string.h>

// Entirely synthetic OAuth replies. Diagnostics never print request forms,
// response bodies, access tokens, refresh tokens or device codes.
#define CHECK(value) {if(!(value)){printf("FAIL OAuth renewal test line %d\n",__LINE__);return false;}}
namespace {
const char CLIENT[]="00000000-0000-4000-8000-000000000001";
const char SCOPES[]="XboxLive.signin offline_access";
const char INITIAL_ACCESS[]="initial-access-token-longer-than-the-renewed-value-12345";
const char INITIAL_REFRESH[]="R%2Binitial%2F%3D%20%26%3F%25";
const char ROTATED_REFRESH[]="R%2Brotated%2F%3D%20%26%3F%25";
const char TRANSFER_REFRESH[]="R%2Btransfer%2F%3D%20%26%3F%25";
const char TRANSFER_SCOPE[]=
    "service%3A%3Ahttp%3A%2F%2FPassport.NET%2Fpurpose%3A%3APURPOSE_XBOX_CLOUD_CONSOLE_TRANSFER_TOKEN";
enum Reply { ROTATE=0,KEEP_REFRESH=1,TRANSFER=2,TLS_FAILURE=3,
             TRANSPORT_FAILURE=4,HTTP_ERROR=5,MALFORMED=6,EMPTY_TOKEN=7,
             BAD_TOKEN=8,ZERO_LIFETIME=9,FRACTIONAL_LIFETIME=10,
             WRONG_TYPE=11,SCOPE_REMOVED=12,BAD_HTTP_STATUS=13 };
struct Fixture {
    unsigned int step;
    Reply reply;
    const char* expected_refresh;
    bool transfer;
    bool no_refresh;
    bool valid_request;
    bool force_unverified;
};
bool respond(Fixture* f,const char* json,int status,
             secure_http::Response* out) {
    memset(out,0,sizeof(*out));
    out->http_status=status;
    out->peer_certificate_verified=true;
    out->hostname_verified=true;
    strcpy_s(out->body,sizeof(out->body),json);
    out->length=strlen(json);
    ++f->step;
    return true;
}
bool synthetic_https(void* context,const char* host,const char* path,
                     const char* form,size_t bytes,secure_http::Response* out) {
    Fixture* f=(Fixture*)context;
    if(!f || !host || !path || !form || !out || bytes!=strlen(form))return false;
    if(f->step==0) {
        f->valid_request=f->valid_request &&
            strcmp(host,"login.microsoftonline.com")==0 &&
            strcmp(path,"/consumers/oauth2/v2.0/devicecode")==0 &&
            strstr(form,CLIENT)!=NULL &&
            strstr(form,"scope=XboxLive.signin%20offline_access")!=NULL;
        return respond(f,"{\"verification_uri\":\"https://www.microsoft.com/link\","
            "\"user_code\":\"SYNTHETIC\",\"device_code\":\"synthetic-device\","
            "\"expires_in\":900,\"interval\":5}",200,out);
    }
    if(f->step==1) {
        f->valid_request=f->valid_request &&
            strcmp(host,"login.microsoftonline.com")==0 &&
            strcmp(path,"/consumers/oauth2/v2.0/token")==0 &&
            strstr(form,"grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Adevice_code")!=NULL &&
            strstr(form,CLIENT)!=NULL;
        char json[1024];
        sprintf_s(json,sizeof(json),
            "{\"token_type\":\"Bearer\",\"access_token\":\"%s\","
            "\"expires_in\":3600,\"scope\":\"XboxLive.signin offline_access\"%s}",
            INITIAL_ACCESS,f->no_refresh?"":",\"refresh_token\":\"R+initial/= &?%\"");
        return respond(f,json,200,out);
    }
    char expected[2048];
    sprintf_s(expected,sizeof(expected),
        "grant_type=refresh_token&client_id=%s&refresh_token=%s&scope=%s",
        CLIENT,f->expected_refresh,
        f->transfer?TRANSFER_SCOPE:"XboxLive.signin%20offline_access");
    f->valid_request=f->valid_request && strcmp(form,expected)==0 &&
        strcmp(host,f->transfer?"login.live.com":"login.microsoftonline.com")==0 &&
        strcmp(path,f->transfer?"/oauth20_token.srf":"/consumers/oauth2/v2.0/token")==0;
    if(!f->valid_request)return false;
    const char* json=NULL;
    int status=200;
    switch(f->reply) {
    case ROTATE:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"renewed\",\"refresh_token\":\"R+rotated/= &?%\",\"expires_in\":3600,\"scope\":\"XboxLive.signin offline_access\"}";break;
    case KEEP_REFRESH:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"kept-token\",\"expires_in\":3600}";break;
    case TRANSFER:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"synthetic-transfer\",\"refresh_token\":\"R+transfer/= &?%\",\"expires_in\":3600}";break;
    case HTTP_ERROR:
        status=400;json="{\"error\":\"invalid_grant\",\"error_description\":\"synthetic private description\"}";break;
    case MALFORMED:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"synthetic-transfer\",\"expires_in\":";break;
    case EMPTY_TOKEN:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"\",\"expires_in\":3600}";break;
    case BAD_TOKEN:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"synthetic\\nheader\",\"expires_in\":3600}";break;
    case ZERO_LIFETIME:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"synthetic-transfer\",\"expires_in\":0}";break;
    case FRACTIONAL_LIFETIME:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"synthetic-transfer\",\"expires_in\":1.5}";break;
    case WRONG_TYPE:
        json="{\"token_type\":\"Basic\",\"access_token\":\"synthetic-transfer\",\"expires_in\":3600}";break;
    case SCOPE_REMOVED:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"renewed\",\"expires_in\":3600,\"scope\":\"offline_access\"}";break;
    default:
        json="{\"token_type\":\"Bearer\",\"access_token\":\"synthetic-transfer\",\"expires_in\":3600}";break;
    }
    const bool sent=respond(f,json,status,out);
    if(f->reply==TLS_FAILURE || f->force_unverified)out->hostname_verified=false;
    if(f->reply==BAD_HTTP_STATUS)out->http_status=700;
    return f->reply==TRANSPORT_FAILURE?false:sent;
}
bool authorize(xbox_device_flow::Flow& flow,secure_http::Transport& tls,
               Fixture& f,bool no_refresh=false) {
    memset(&f,0,sizeof(f));
    f.expected_refresh=INITIAL_REFRESH;
    f.valid_request=true;
    f.no_refresh=no_refresh;
    tls.attach(synthetic_https,&f);
    CHECK(flow.configure(CLIENT,SCOPES,&tls));
    CHECK(flow.start(1000));
    CHECK(flow.poll(6000));
    CHECK(flow.is_authorized() && flow.xbox_scope_granted());
    CHECK(f.step==2 && f.valid_request);
    CHECK(strcmp(flow.xbox_token_for_worker(),INITIAL_ACCESS)==0);
    return true;
}
bool all_zero(const char* value,size_t bytes) {
    for(size_t i=0;i<bytes;++i)if(value[i]!=0)return false;
    return true;
}
bool rotation_and_transfer() {
    xbox_device_flow::Flow flow;secure_http::Transport tls;Fixture f;
    CHECK(authorize(flow,tls,f));
    const char* original=flow.xbox_token_for_worker();
    const size_t previous_length=strlen(original);
    f.reply=ROTATE;
    CHECK(flow.refresh_for_worker(7000));
    CHECK(f.step==3 && f.valid_request);
    CHECK(strcmp(flow.xbox_token_for_worker(),"renewed")==0);
    CHECK(all_zero(original+strlen("renewed"),previous_length-strlen("renewed")));
    f.expected_refresh=ROTATED_REFRESH;f.reply=KEEP_REFRESH;
    CHECK(flow.refresh_for_worker(8000));
    CHECK(flow.xbox_scope_granted()); // Absent scope retains consent.
    CHECK(strcmp(flow.xbox_token_for_worker(),"kept-token")==0);
    char transfer[128];memset(transfer,0x55,sizeof(transfer));
    f.transfer=true;f.reply=TRANSFER;
    CHECK(flow.request_transfer_token(transfer,sizeof(transfer)));
    CHECK(strcmp(transfer,"synthetic-transfer")==0 && f.valid_request);
    CHECK(strcmp(flow.xbox_token_for_worker(),"kept-token")==0);
    f.transfer=false;f.expected_refresh=TRANSFER_REFRESH;f.reply=KEEP_REFRESH;
    CHECK(flow.refresh_for_worker(9000)); // Transfer grant also rotates refresh.
    CHECK(f.valid_request && f.step==6);
    flow.cancel();
    CHECK(!flow.is_authorized() && !flow.xbox_token_for_worker());
    CHECK(all_zero(original,previous_length));
    secure_http::secure_zero(transfer,sizeof(transfer));
    return true;
}
bool failed_renewals() {
    const Reply failures[]={TLS_FAILURE,TRANSPORT_FAILURE,HTTP_ERROR,MALFORMED,
        EMPTY_TOKEN,BAD_TOKEN,ZERO_LIFETIME,FRACTIONAL_LIFETIME,WRONG_TYPE,BAD_HTTP_STATUS};
    for(unsigned int i=0;i<sizeof(failures)/sizeof(failures[0]);++i) {
        xbox_device_flow::Flow flow;secure_http::Transport tls;Fixture f;
        CHECK(authorize(flow,tls,f));
        f.reply=failures[i];f.transfer=true;
        char output[128];memset(output,0x55,sizeof(output));
        CHECK(!flow.request_transfer_token(output,sizeof(output)));
        CHECK(!flow.reauthentication_required());
        CHECK(all_zero(output,sizeof(output)));
        CHECK(f.valid_request && f.step==3);
        CHECK(flow.xbox_scope_granted());
        CHECK(strcmp(flow.xbox_token_for_worker(),INITIAL_ACCESS)==0);
        // A failed grant must not replace either original credential.
        f.transfer=false;f.reply=KEEP_REFRESH;
        CHECK(flow.refresh_for_worker(8000));
        CHECK(f.valid_request && f.step==4);
    }
    return true;
}
struct Persisted {
    unsigned int count;
    bool valid;
    bool allow;
    char refresh[8192];
};
bool capture_cache(void* context,const char* client,const char* requested,
                   const char* granted,const char* refresh) {
    Persisted* cache=(Persisted*)context;
    ++cache->count;
    cache->valid=cache->valid && strcmp(client,CLIENT)==0 && strcmp(requested,SCOPES)==0 &&
        granted!=NULL && refresh && refresh[0];
    strcpy_s(cache->refresh,sizeof(cache->refresh),refresh);
    return cache->allow;
}
bool cached_resume() {
    xbox_device_flow::Flow flow;secure_http::Transport tls;Fixture f;
    Persisted stored={0,true,true,{0}};
    flow.set_persistence(capture_cache,&stored);
    CHECK(authorize(flow,tls,f));
    CHECK(stored.count==1 && stored.valid && strcmp(stored.refresh,"R+initial/= &?%")==0);
    // Reconstruct a fresh process flow: refresh grant only, no device-code call.
    xbox_device_flow::Flow restored;
    CHECK(restored.configure(CLIENT,SCOPES,&tls));
    restored.set_persistence(capture_cache,&stored);
    f.step=2;f.reply=ROTATE;f.valid_request=true;f.expected_refresh=INITIAL_REFRESH;
    CHECK(restored.resume_from_refresh(stored.refresh,SCOPES,10000));
    CHECK(f.step==3 && f.valid_request && restored.is_authorized() && restored.xbox_scope_granted());
    CHECK(stored.count==2 && strcmp(stored.refresh,"R+rotated/= &?%")==0);
    // Saving failure must not discard a live authenticated session.
    stored.allow=false;f.expected_refresh=ROTATED_REFRESH;f.reply=KEEP_REFRESH;
    CHECK(restored.refresh_for_worker(11000) && restored.is_authorized());
    CHECK(stored.count==3 && stored.valid);
    secure_http::secure_zero(stored.refresh,sizeof(stored.refresh));
    const Reply failures[]={TRANSPORT_FAILURE,TLS_FAILURE,HTTP_ERROR};
    for(unsigned int n=0;n<3;++n) {
        xbox_device_flow::Flow retry;
        CHECK(retry.configure(CLIENT,SCOPES,&tls));
        f.step=2;f.reply=failures[n];f.expected_refresh=INITIAL_REFRESH;f.valid_request=true;
        CHECK(!retry.resume_from_refresh("R+initial/= &?%",SCOPES,12000));
        CHECK(f.step==3 && f.valid_request && !retry.is_authorized());
        CHECK(retry.reauthentication_required()==(failures[n]==HTTP_ERROR));
    }
    xbox_device_flow::Flow unverified;
    CHECK(unverified.configure(CLIENT,SCOPES,&tls));
    f.step=2;f.reply=HTTP_ERROR;f.force_unverified=true;f.valid_request=true;
    CHECK(!unverified.resume_from_refresh("R+initial/= &?%",SCOPES,12000));
    CHECK(!unverified.reauthentication_required());
    f.force_unverified=false;
    return true;
}

bool scope_capacity_and_missing_refresh() {
    xbox_device_flow::Flow flow;secure_http::Transport tls;Fixture f;
    CHECK(authorize(flow,tls,f));
    f.transfer=true;f.reply=TRANSFER;
    char output[4];memset(output,0x55,sizeof(output));
    CHECK(!flow.request_transfer_token(output,sizeof(output)));
    CHECK(all_zero(output,sizeof(output)));
    // Output failure cannot commit the rotated transfer refresh token.
    f.transfer=false;f.reply=SCOPE_REMOVED;
    CHECK(flow.refresh_for_worker(8000));
    CHECK(f.valid_request && !flow.xbox_scope_granted() && !flow.xbox_token_for_worker());
    CHECK(authorize(flow,tls,f,true));
    memset(output,0x55,sizeof(output));
    CHECK(!flow.request_transfer_token(output,sizeof(output)));
    CHECK(all_zero(output,sizeof(output)) && f.step==2);
    CHECK(!flow.refresh_for_worker(9000) && f.step==2);
    CHECK(!flow.request_transfer_token(NULL,100));
    flow.cancel();
    memset(output,0x55,sizeof(output));
    CHECK(!flow.request_transfer_token(output,sizeof(output)) && all_zero(output,sizeof(output)));
    return true;
}
}
int main() {
    if(!xbox_device_flow::offline_selftest() || !rotation_and_transfer() ||
       !failed_renewals() || !cached_resume() || !scope_capacity_and_missing_refresh())return 1;
    puts("PASS: OAuth rotation, cached resume without device code, persistence callbacks, transient failure retention, verified reauthentication and credential clearing.");
    return 0;
}
