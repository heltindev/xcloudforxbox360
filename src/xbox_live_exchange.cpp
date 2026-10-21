#include "xbox_live_exchange.h"
#include "xbox_live_auth.h"
#include "cJSON.h"
#include "region_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace xbox_live_exchange {
namespace {
const char USER_HOST[]="user.auth.xboxlive.com";
const char USER_PATH[]="/user/authenticate";
const char XSTS_HOST[]="xsts.auth.xboxlive.com";
const char XSTS_PATH[]="/xsts/authorize";
// The xCloud-specific relying party requires its own Xbox Live permission;
// having an XSTS token cannot bypass xCloud entitlement or session access.
const char CLOUD_HOST[]="xgpuweb.gssv-play-prod.xboxlive.com";
const char CLOUD_PATH[]="/v2/login/user";
const char CLOUD_RELYING_PARTY[]="http://gssv.xboxlive.com/";
struct Workspace {
    char form[8192];
    char xbox_user[8192];
    secure_http::Response http;
};
void initial(Result* out) {
    memset(out,0,sizeof(*out));
    out->stage=NOT_STARTED;
}
bool valid_server_token(const char* token) {
    if(!token)return false;
    size_t n=0;
    for(;n<7000 && token[n];++n) {
        const unsigned char c=(unsigned char)token[n];
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||
             c=='+'||c=='/'||c=='='||c=='~'))return false;
    }
    return n>=12 && n<7000 && token[n]==0;
}
// Xbox replies contain nested DisplayClaims; the flat OAuth parser rejects
// these valid replies. Parse an exact-length object and wipe all string copies.
void erase_json(cJSON* item) {
    if(!item)return;
    for(cJSON* child=item->child;child;child=child->next)erase_json(child);
    if(item->valuestring)
        secure_http::secure_zero(item->valuestring,strlen(item->valuestring));
}
void release_json(cJSON* root) {erase_json(root);cJSON_Delete(root);}
cJSON* response_json(const secure_http::Response& response) {
    if(!response.length || response.length>=secure_http::RESPONSE_CAP)return NULL;
    const char* end=NULL;
    cJSON* root=cJSON_ParseWithLengthOpts(response.body,response.length,&end,0);
    if(!root)return NULL;
    while(end && end<response.body+response.length &&
          (*end==' ' || *end=='\r' || *end=='\n' || *end=='\t'))++end;
    if(end!=response.body+response.length || !cJSON_IsObject(root)) {
        release_json(root);return NULL;
    }
    // Reject duplicate top-level keys rather than silently choosing a token.
    for(const cJSON* x=root->child;x;x=x->next)
        for(const cJSON* y=x->next;y;y=y->next)
            if(x->string && y->string && strcmp(x->string,y->string)==0) {
                release_json(root);return NULL;
            }
    return root;
}
void server_error(const secure_http::Response& response,Result* out) {
    cJSON* root=response_json(response);
    if(!root)return;
    const cJSON* code=cJSON_GetObjectItemCaseSensitive(root,"XErr");
    if(cJSON_IsNumber(code) && code->valuedouble>=0.0 &&
       code->valuedouble<=4294967295.0) {
        const unsigned int n=(unsigned int)code->valuedouble;
        if((double)n==code->valuedouble)out->xerr=n;
    }
    release_json(root);
}
bool verified_response(const secure_http::Response& response) {
    return response.peer_certificate_verified &&
           response.hostname_verified &&
           response.length>0 &&
           response.length<secure_http::RESPONSE_CAP &&
           response.http_status>=100 && response.http_status<=599;
}
bool validate_cloud(const secure_http::Response& response,unsigned int* count,
                    WorkerContext* worker) {
    *count=0;
    cJSON* root=response_json(response);
    if(!root)return false;
    const cJSON* token=cJSON_GetObjectItemCaseSensitive(root,"gsToken");
    const cJSON* duration=cJSON_GetObjectItemCaseSensitive(root,"durationInSeconds");
    const cJSON* settings=cJSON_GetObjectItemCaseSensitive(root,"offeringSettings");
    const cJSON* regions=cJSON_GetObjectItemCaseSensitive(settings,"regions");
    bool okay=cJSON_IsString(token) && valid_server_token(token->valuestring) &&
        cJSON_IsNumber(duration) && duration->valuedouble>0.0 &&
        duration->valuedouble<=2147483647.0 &&
        duration->valuedouble==(double)(unsigned int)duration->valuedouble &&
        cJSON_IsArray(regions);
    const char* selected_uri=NULL;
    bool selected_default=false;
    if(okay) {
        const cJSON* item=NULL;
        cJSON_ArrayForEach(item,regions) {
            const cJSON* uri=cJSON_GetObjectItemCaseSensitive(item,"baseUri");
            char host[128];
            if(cJSON_IsString(uri) &&
               region_catalog::extract_xbox_hostname(uri->valuestring,host,sizeof(host))) {
                ++*count;
                const bool is_default=cJSON_IsTrue(
                    cJSON_GetObjectItemCaseSensitive(item,"isDefault"))!=0;
                if(!selected_uri || (is_default && !selected_default)) {
                    selected_uri=uri->valuestring;
                    selected_default=is_default;
                }
            }
        }
        okay=*count>0;
        if(okay && worker)
            okay=worker->credentials->retain(token->valuestring,selected_uri,
                (unsigned int)duration->valuedouble,
                worker->clock_ms(worker->context));
    }
    release_json(root);
    if(!okay)*count=0;
    return okay;
}
bool get_token(const secure_http::Response& response,
               char* out,size_t capacity) {
    if(!out || !capacity)return false;
    out[0]=0;
    cJSON* root=response_json(response);
    if(!root)return false;
    const cJSON* token=cJSON_GetObjectItemCaseSensitive(root,"Token");
    const bool okay=cJSON_IsString(token) &&
        valid_server_token(token->valuestring) &&
        strlen(token->valuestring)<capacity;
    if(okay)strcpy_s(out,capacity,token->valuestring);
    release_json(root);
    return okay;
}
}
CloudCredentials::CloudCredentials() {clear();}
CloudCredentials::~CloudCredentials() {clear();}
void CloudCredentials::clear() {
    secure_http::secure_zero(token_,sizeof(token_));
    secure_http::secure_zero(host_,sizeof(host_));
    started_ms_=0;lifetime_ms_=0;
}
bool CloudCredentials::retain(const char* token,const char* uri,
                              unsigned int seconds,unsigned int now) {
    clear();
    if(!seconds || !valid_server_token(token) ||
       !region_catalog::extract_xbox_hostname(uri,host_,sizeof(host_))) {
        clear();return false;
    }
    strcpy_s(token_,sizeof(token_),token);
    // Bound retention to one day, below the 32-bit elapsed-time ambiguity.
    if(seconds>86400u)seconds=86400u;
    started_ms_=now;lifetime_ms_=seconds*1000u;
    return true;
}
unsigned int CloudCredentials::remaining_seconds(unsigned int now) {
    const unsigned int elapsed=now-started_ms_;
    if(!lifetime_ms_ || elapsed>=lifetime_ms_) {clear();return 0;}
    return (lifetime_ms_-elapsed+999u)/1000u;
}
const char* CloudCredentials::token_for_worker(unsigned int now) {
    return remaining_seconds(now)?token_:NULL;
}
const char* CloudCredentials::host_for_worker(unsigned int now) {
    return remaining_seconds(now)?host_:NULL;
}
const char* stage_text(Stage stage) {
    switch(stage) {
    case EXCHANGE_CANCELLED:return "LOGIN CANCELLED";
    case NOT_STARTED:return "XBOX EXCHANGE NOT STARTED";
    case XBOX_SCOPE_MISSING:return "XBOX SCOPE NOT GRANTED";
    case INVALID_TOKEN:return "MICROSOFT TOKEN NOT USABLE";
    case HTTPS_UNAVAILABLE:return "XBOX HTTPS NOT VERIFIED / NETWORK ERROR";
    case XBOX_USER_HTTP_REJECTED:return "XBOX USER TOKEN REJECTED";
    case XBOX_USER_RESPONSE_INVALID:return "XBOX USER TOKEN RESPONSE INVALID";
    case XSTS_HTTP_REJECTED:return "XSTS REQUEST REJECTED";
    case XSTS_RESPONSE_INVALID:return "XSTS RESPONSE INVALID";
    case CLOUD_HTTP_REJECTED:return "XCLOUD LOGIN REJECTED";
    case CLOUD_RESPONSE_INVALID:return "XCLOUD LOGIN RESPONSE INVALID";
    case CLOUD_LOGIN_COMPLETE:return "XCLOUD LOGIN OK - MEDIA NOT IMPLEMENTED";
    case XSTS_COMPLETE:return "XSTS GRANTED - XCLOUD SESSION STILL PENDING";
    default:return "UNRECOGNIZED EXCHANGE STAGE";
    }
}
bool exchange(const char* msa,bool scope,PostJson callback,
              void* context,Result* out,WorkerContext* worker) {
    if(worker && worker->credentials)worker->credentials->clear();
    if(!out)return false;
    initial(out);
    if(worker && (!worker->credentials || !worker->clock_ms)) {
        out->stage=HTTPS_UNAVAILABLE;return false;
    }
    if(!scope) {out->stage=XBOX_SCOPE_MISSING;return false;}
    if(!valid_server_token(msa)) {
        out->stage=INVALID_TOKEN;return false;
    }
    if(!callback) {out->stage=HTTPS_UNAVAILABLE;return false;}
    // Never put the OAuth token, Xbox User token or XSTS token into
    // UI state, persistent storage, diagnostic traces or function results.
    Workspace* scratch=(Workspace*)malloc(sizeof(Workspace));
    if(!scratch){out->stage=HTTPS_UNAVAILABLE;return false;}
    memset(scratch,0,sizeof(*scratch));
    bool success=false;
    // Exactly one attempt; an early break still reaches the common secure wipe.
    // A bounded loop avoids MSVC C4127 from do/while(false).
    for (int attempt=0;attempt<1;++attempt) {
        if(worker && worker->cancelled && worker->cancelled(worker->context)) {
            out->stage=EXCHANGE_CANCELLED;break;
        }
        if(!xbox_live_auth::make_user_auth_request(msa,
                 scratch->form,sizeof(scratch->form))) {
            out->stage=INVALID_TOKEN;break;
        }
        const size_t user_request=strlen(scratch->form);
        const bool user_sent=callback(context,USER_HOST,USER_PATH,
            scratch->form,user_request,&scratch->http);
        secure_http::secure_zero(scratch->form,sizeof(scratch->form));
        if(!user_sent || !verified_response(scratch->http)) {
            out->stage=HTTPS_UNAVAILABLE;break;
        }
        out->user_http=scratch->http.http_status;
        if(scratch->http.http_status!=200) {
            server_error(scratch->http,out);
            out->stage=XBOX_USER_HTTP_REJECTED;break;
        }
        if(!get_token(scratch->http,scratch->xbox_user,
                      sizeof(scratch->xbox_user))) {
            out->stage=XBOX_USER_RESPONSE_INVALID;break;
        }
        out->xbox_user_received=true;
        secure_http::secure_zero(&scratch->http,sizeof(scratch->http));
        if(!xbox_live_auth::make_xsts_request_for_service(
                scratch->xbox_user,CLOUD_RELYING_PARTY,
                scratch->form,sizeof(scratch->form))) {
            out->stage=XBOX_USER_RESPONSE_INVALID;break;
        }
        secure_http::secure_zero(scratch->xbox_user,
                                 sizeof(scratch->xbox_user));
        const size_t xsts_request=strlen(scratch->form);
        if(worker && worker->cancelled && worker->cancelled(worker->context)) {
            out->stage=EXCHANGE_CANCELLED;break;
        }
        const bool xsts_sent=callback(context,XSTS_HOST,XSTS_PATH,
            scratch->form,xsts_request,&scratch->http);
        secure_http::secure_zero(scratch->form,sizeof(scratch->form));
        if(!xsts_sent || !verified_response(scratch->http)) {
            out->stage=HTTPS_UNAVAILABLE;break;
        }
        out->xsts_http=scratch->http.http_status;
        if(scratch->http.http_status!=200) {
            server_error(scratch->http,out);
            out->stage=XSTS_HTTP_REJECTED;break;
        }
        // Do not copy XSTS token to a long-lived buffer until an
        // authorized session module exists. Validate the returned format
        // transiently and erase it immediately.
        if(!get_token(scratch->http,scratch->xbox_user,
                      sizeof(scratch->xbox_user))) {
            out->stage=XSTS_RESPONSE_INVALID;break;
        }
        out->xsts_received=true;
        out->stage=XSTS_COMPLETE;
        // Continue while the service-specific XSTS token is still in worker
        // memory; it never enters the UI snapshot or persistent storage.
        const int n=sprintf_s(scratch->form,sizeof(scratch->form),
            "{\"token\":\"%s\",\"offeringId\":\"xgpuweb\"}",scratch->xbox_user);
        secure_http::secure_zero(scratch->xbox_user,sizeof(scratch->xbox_user));
        secure_http::secure_zero(&scratch->http,sizeof(scratch->http));
        if(n<=0 || (size_t)n>=sizeof(scratch->form)) {
            out->stage=INVALID_TOKEN;break;
        }
        if(worker && worker->cancelled && worker->cancelled(worker->context)) {
            out->stage=EXCHANGE_CANCELLED;break;
        }
        const bool cloud_sent=callback(context,CLOUD_HOST,CLOUD_PATH,
            scratch->form,(size_t)n,&scratch->http);
        secure_http::secure_zero(scratch->form,sizeof(scratch->form));
        if(!cloud_sent || !verified_response(scratch->http)) {
            out->stage=HTTPS_UNAVAILABLE;break;
        }
        out->cloud_http=scratch->http.http_status;
        if(out->cloud_http!=200) {out->stage=CLOUD_HTTP_REJECTED;break;}
        if(worker && worker->cancelled && worker->cancelled(worker->context)) {
            out->stage=EXCHANGE_CANCELLED;break;
        }
        if(!validate_cloud(scratch->http,&out->cloud_regions,worker)) {
            out->stage=CLOUD_RESPONSE_INVALID;break;
        }
        out->cloud_login_received=true;
        out->stage=CLOUD_LOGIN_COMPLETE;
        // No cloud VM is allocated without a working WebRTC/media runtime.
        // Only the worker-owned credential survives; response copies are wiped.
        success=true;
    }
    if(!success && worker)worker->credentials->clear();
    secure_http::secure_zero(scratch,sizeof(*scratch));
    free(scratch);
    return success;
}

namespace {
struct Mock {
    unsigned int step;
    bool verified;
    bool fail_xsts;
    int cloud_mode; // 0=valid, 1=HTTP rejection, 2=invalid region, 3=unverified
};
bool fake_post(void* ctx,const char* host,const char* path,
               const char* json,size_t bytes,secure_http::Response* result) {
    Mock* mock=(Mock*)ctx;
    if(!mock || !result || !json || !bytes)return false;
    memset(result,0,sizeof(*result));
    const char* reply=0;
    if(mock->step==0 && strcmp(host,USER_HOST)==0 &&
       strcmp(path,USER_PATH)==0 &&
       strstr(json,"\"RpsTicket\":\"d=sample-msa-token-123\"")) {
        reply="{\"IssueInstant\":\"2026-10-21T00:00:00Z\",\"NotAfter\":\"2026-10-22T00:00:00Z\",\"Token\":\"mock-user-token-1234\",\"DisplayClaims\":{\"xui\":[{\"uhs\":\"1234567890\"}]}}";
        result->http_status=200;
    } else if(mock->step==1 && strcmp(host,XSTS_HOST)==0 &&
              strcmp(path,XSTS_PATH)==0 &&
              strstr(json,"\"http://gssv.xboxlive.com/\"")) {
        if(mock->fail_xsts) {
            reply="{\"XErr\":2148916233,\"Message\":\"Forbidden\",\"Details\":{\"reason\":\"account\"}}";
            result->http_status=401;
        } else {
            reply="{\"Token\":\"mock-xsts-token-1234\",\"DisplayClaims\":{\"xui\":[{\"uhs\":\"1234567890\",\"xid\":\"0000000000\"}]}}";
            result->http_status=200;
        }
    } else if(mock->step==2 && strcmp(host,CLOUD_HOST)==0 &&
              strcmp(path,CLOUD_PATH)==0 &&
              strstr(json,"\"token\":\"mock-xsts-token-1234\"") &&
              strstr(json,"\"offeringId\":\"xgpuweb\"")) {
        result->http_status=mock->cloud_mode==1?403:200;
        reply=mock->cloud_mode==1 ? "{\"error\":\"Forbidden\"}" :
            (mock->cloud_mode==2 ?
            "{\"gsToken\":\"mock-cloud-token-1234\",\"durationInSeconds\":3600,\"offeringSettings\":{\"regions\":[{\"baseUri\":\"https://evil.example\"}]}}" :
            "{\"gsToken\":\"mock-cloud-token-1234\",\"durationInSeconds\":3600,\"offeringSettings\":{\"regions\":[{\"baseUri\":\"https://brs.gssv-play-prod.xboxlive.com\"}]}}");
    } else return false;
    const size_t n=strlen(reply);
    memcpy(result->body,reply,n);
    result->length=n;
    result->peer_certificate_verified=mock->verified && !(mock->step==2 && mock->cloud_mode==3);
    result->hostname_verified=mock->verified;
    ++mock->step;
    return true;
}
}
bool offline_selftest() {
    Result status;
    Mock mock={0,true,false,0};
    if(exchange("sample-msa-token-123",false,fake_post,&mock,&status) ||
       status.stage!=XBOX_SCOPE_MISSING || mock.step!=0)return false;
    if(!exchange("sample-msa-token-123",true,fake_post,&mock,&status) ||
       status.stage!=CLOUD_LOGIN_COMPLETE || !status.cloud_login_received ||
       status.cloud_regions!=1 || status.cloud_http!=200 || !status.xbox_user_received ||
       !status.xsts_received || status.user_http!=200 ||
       status.xsts_http!=200 || mock.step!=3)return false;
    mock.step=0;mock.verified=false;
    if(exchange("sample-msa-token-123",true,fake_post,&mock,&status) ||
       status.stage!=HTTPS_UNAVAILABLE ||
       status.xbox_user_received)return false;
    mock.step=0;mock.verified=true;mock.fail_xsts=true;
    if(exchange("sample-msa-token-123",true,fake_post,&mock,&status) ||
       status.stage!=XSTS_HTTP_REJECTED ||
       status.xerr!=2148916233u ||
       !status.xbox_user_received || status.xsts_received)return false;

    // Real Xbox response shape includes objects/arrays; flat JSON is insufficient.
    secure_http::Response* nested=(secure_http::Response*)malloc(sizeof(secure_http::Response));
    if(!nested)return false;
    memset(nested,0,sizeof(*nested));
    char extracted[128];
    const char* bad[]={
        "{\"Token\":\"mock-xsts-token-1234\",\"Token\":\"other-token-value\"}",
        "{\"Token\":\"mock-xsts-token-1234\"} trailing",
        "{\"Token\":\"mock-xsts-token-1234\",\"DisplayClaims\":{",
        "{\"Token\":{\"value\":\"mock-xsts-token-1234\"}}"
    };
    bool rejected=true;
    for(unsigned int n=0;n<sizeof(bad)/sizeof(bad[0]);++n) {
        strcpy_s(nested->body,sizeof(nested->body),bad[n]);
        nested->length=strlen(bad[n]);
        if(get_token(*nested,extracted,sizeof(extracted)))rejected=false;
    }
    secure_http::secure_zero(extracted,sizeof(extracted));
    secure_http::secure_zero(nested,sizeof(*nested));free(nested);
    if(!rejected)return false;
    for(int mode=1;mode<=3;++mode) {
        mock.step=0;mock.verified=true;mock.fail_xsts=false;mock.cloud_mode=mode;
        if(exchange("sample-msa-token-123",true,fake_post,&mock,&status) ||
           status.cloud_login_received || !status.xsts_received)return false;
        if(mode==1 && (status.stage!=CLOUD_HTTP_REJECTED || status.cloud_http!=403))
            return false;
        if(mode==2 && status.stage!=CLOUD_RESPONSE_INVALID)return false;
        if(mode==3 && status.stage!=HTTPS_UNAVAILABLE)return false;
    }
    return true;
}
}
