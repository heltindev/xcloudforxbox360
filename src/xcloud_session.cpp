#include "xcloud_session.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <new>

namespace xcloud_session {
namespace {
const char PREFIX[]="/v5/sessions/cloud/";
const char PLAY[]="/v5/sessions/cloud/play";
bool hex(char c) {
    return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');
}
bool host_allowed(const char* host) {
    if(!host)return false;
    const size_t n=strlen(host);
    const char suffix[]=".gssv-play-prod.xboxlive.com";
    const size_t tail=sizeof(suffix)-1;
    if(n<=tail || n>=128 || strcmp(host+n-tail,suffix)!=0)return false;
    size_t label=0;
    for(size_t i=0;i<n;++i) {
        const char c=host[i];
        if(c=='.') {
            if(label==0 || host[i-1]=='-')return false;
            label=0;
        } else {
            if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-') ||
               (!label && c=='-') || ++label>63)return false;
        }
    }
    return label>0 && host[n-1]!='-';
}
bool token_valid(const char* token) {
    if(!token)return false;
    size_t n=0;
    for(;n<7168 && token[n];++n) {
        const unsigned char c=(unsigned char)token[n];
        // Tokens are opaque but must be safe both in a header and JSON.
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
             (c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||
             c=='+'||c=='/'||c=='='||c=='~'))return false;
    }
    return n>=12 && n<7168;
}
void erase_json(cJSON* item) {
    if(!item)return;
    for(cJSON* child=item->child;child;child=child->next)erase_json(child);
    if(item->valuestring)
        secure_http::secure_zero(item->valuestring,strlen(item->valuestring));
}
void free_json(cJSON* root) {erase_json(root);cJSON_Delete(root);}
bool unique_keys(const cJSON* object) {
    for(const cJSON* x=object->child;x;x=x->next) {
        if(cJSON_IsObject(object)) {
            for(const cJSON* y=x->next;y;y=y->next)
                if(x->string && y->string && strcmp(x->string,y->string)==0)
                    return false;
        }
        if((cJSON_IsObject(x)||cJSON_IsArray(x)) && !unique_keys(x))return false;
    }
    return true;
}
cJSON* parse(const secure_http::Response& response) {
    if(!response.length || response.length>=secure_http::RESPONSE_CAP)return NULL;
    const char* end=NULL;
    cJSON* root=cJSON_ParseWithLengthOpts(response.body,response.length,&end,0);
    if(!root)return NULL;
    while(end && end<response.body+response.length &&
          (*end==' '||*end=='\t'||*end=='\r'||*end=='\n'))++end;
    if(end!=response.body+response.length || !cJSON_IsObject(root) ||
       !unique_keys(root)) {
        free_json(root);return NULL;
    }
    return root;
}
const char* string_field(const cJSON* object,const char* key) {
    const cJSON* value=cJSON_GetObjectItemCaseSensitive(object,key);
    return cJSON_IsString(value)?value->valuestring:NULL;
}
void error_code(const secure_http::Response& response,char* out,size_t capacity) {
    cJSON* root=parse(response);
    if(!root)return;
    const char* code=string_field(root,"code");
    if(!code)code=string_field(root,"error");
    // A short diagnostic code can be exposed; arbitrary messages, responses,
    // nested credentials and server URLs cannot enter UI snapshots/logs.
    if(code && code[0] && strlen(code)<capacity) {
        bool safe=true;
        for(size_t i=0;code[i];++i)
            if(!((code[i]>='A'&&code[i]<='Z')||
                 (code[i]>='a'&&code[i]<='z')||
                 (code[i]>='0'&&code[i]<='9')||code[i]=='_'))safe=false;
        if(safe)strcpy_s(out,capacity,code);
    }
    free_json(root);
}
}
bool valid_session_path(const char* path) {
    if(!path || strlen(path)!=sizeof(PREFIX)-1+36 ||
       strncmp(path,PREFIX,sizeof(PREFIX)-1)!=0)return false;
    const char* id=path+sizeof(PREFIX)-1;
    for(unsigned int i=0;i<36;++i) {
        if(i==8||i==13||i==18||i==23) {if(id[i]!='-')return false;}
        else if(!hex(id[i]))return false;
    }
    return true;
}
bool normalize_session_path(const char* input,char* out,size_t capacity) {
    if(!input||!out||!capacity)return false;
    out[0]=0;
    const size_t n=strlen(input);
    if(input[0]=='/') {
        if(!valid_session_path(input) || n>=capacity)return false;
        memcpy(out,input,n+1);return true;
    }
    if(n+1>=capacity || n!=sizeof(PREFIX)-2+36)return false;
    out[0]='/';memcpy(out+1,input,n+1);
    if(!valid_session_path(out)) {out[0]=0;return false;}
    return true;
}
bool validate_request(const char* method,const char* host,const char* path) {
    if(!method || !path || !host_allowed(host))return false;
    if(strcmp(method,"POST")==0 && strcmp(path,PLAY)==0)return true;
    const size_t root_length=sizeof(PREFIX)-1+36;
    const size_t n=strlen(path);
    if(n<root_length || n>=96)return false;
    char root[64];
    memcpy(root,path,root_length);root[root_length]=0;
    if(!valid_session_path(root))return false;
    const char* action=path+root_length;
    return (strcmp(method,"DELETE")==0 && !action[0]) ||
           (strcmp(method,"GET")==0 && strcmp(action,"/state")==0) ||
           (strcmp(method,"POST")==0 && strcmp(action,"/connect")==0);
}
Session::Session():callback_(NULL),context_(NULL) {
    memset(host_,0,sizeof(host_));memset(path_,0,sizeof(path_));
    memset(&state_,0,sizeof(state_));state_.stage=IDLE;
}
Session::~Session() {
    // Network cleanup is explicit: a destructor cannot safely obtain an
    // unexpired credential or block while the worker is shutting down.
    secure_http::secure_zero(host_,sizeof(host_));
    secure_http::secure_zero(path_,sizeof(path_));
}
void Session::attach(Request callback,void* context) {
    if(needs_cleanup())return;
    callback_=callback;context_=context;
}
bool Session::fail(const char* reason) {
    state_.stage=FAILED;
    strcpy_s(state_.error_code,sizeof(state_.error_code),reason);
    return false;
}
bool Session::request(const char* method,const char* suffix,const char* token,
                      const char* json,size_t bytes,
                      secure_http::Response* response) {
    if(!response)return fail("SESSION_NO_MEMORY");
    memset(response,0,sizeof(*response));
    state_.http_status=0;
    char path[96];
    const int n=strcmp(suffix,PLAY)==0?
        sprintf_s(path,sizeof(path),"%s",PLAY):
        sprintf_s(path,sizeof(path),"%s%s",path_,suffix);
    if(n<0 || !validate_request(method,host_,path))return fail("SESSION_ROUTE_INVALID");
    if(!token_valid(token))return fail("SESSION_CREDENTIAL_REQUIRED");
    if(!callback_)return fail("SESSION_HTTPS_UNAVAILABLE");
    if(!callback_(context_,method,host_,path,token,json,bytes,response))
        return fail("SESSION_TRANSPORT_FAILED");
    if(!response->peer_certificate_verified || !response->hostname_verified ||
       response->length>=secure_http::RESPONSE_CAP ||
       response->http_status<100 || response->http_status>599)
        return fail("SESSION_HTTPS_UNVERIFIED");
    state_.http_status=response->http_status;
    if(response->http_status<200 || response->http_status>=300) {
        fail(response->http_status==401 || response->http_status==403?
             "SESSION_ACCESS_REFUSED":"SESSION_HTTP_REJECTED");
        error_code(*response,state_.error_code,sizeof(state_.error_code));
        return false;
    }
    return true;
}
bool Session::start(const char* host,const char* token,const char* title) {
    if(needs_cleanup())return fail("SESSION_CLEANUP_REQUIRED");
    memset(&state_,0,sizeof(state_));state_.stage=CREATING;
    memset(path_,0,sizeof(path_));
    if(!host_allowed(host))return fail("SESSION_HOST_INVALID");
    strcpy_s(host_,sizeof(host_),host);
    // FORTNITE is the xCloud catalog titleId. BT5P2X999VH2 is the store
    // productId and must not be substituted for this service identifier.
    if(!title || strcmp(title,"FORTNITE")!=0)return fail("SESSION_TITLE_UNSUPPORTED");
    const char body[]="{\"titleId\":\"FORTNITE\",\"clientSessionId\":\"\","
        "\"settings\":{\"nanoVersion\":\"V3;WebrtcTransport.dll\","
        "\"enableTextToSpeech\":false,\"highContrast\":0,"
        "\"locale\":\"pt-BR\",\"useIceConnection\":true,"
        "\"timezoneOffsetMinutes\":180},\"serverId\":\"\"}";
    secure_http::Response* response=new(std::nothrow) secure_http::Response;
    if(!response)return fail("SESSION_NO_MEMORY");
    bool okay=request("POST",PLAY,token,body,sizeof(body)-1,response);
    cJSON* root=okay?parse(*response):NULL;
    if(okay) {
        const char* returned_path=root?string_field(root,"sessionPath"):NULL;
        if(!normalize_session_path(returned_path,path_,sizeof(path_)))
            okay=fail("SESSION_RESPONSE_INVALID");
        else {
            state_.session_created=true;state_.cleanup_pending=true;
            state_.stage=PROVISIONING;
        }
    }
    free_json(root);secure_http::secure_zero(response,sizeof(*response));
    delete response;return okay;
}
bool Session::poll(const char* token) {
    if(!needs_cleanup())return fail("SESSION_NOT_CREATED");
    if(state_.stage==FAILED)return false;
    if(state_.stage==PROVISIONED)return true;
    if(state_.polls>=MAX_STATE_POLLS)return fail("SESSION_WAIT_LIMIT");
    secure_http::Response* response=new(std::nothrow) secure_http::Response;
    if(!response)return fail("SESSION_NO_MEMORY");
    ++state_.polls;
    bool okay=request("GET","/state",token,NULL,0,response);
    cJSON* root=okay?parse(*response):NULL;
    if(okay) {
        const char* name=root?string_field(root,"state"):NULL;
        if(!name)okay=fail("SESSION_STATE_INVALID");
        else if(strcmp(name,"Provisioned")==0)state_.stage=PROVISIONED;
        else if(strcmp(name,"ReadyToConnect")==0)
            state_.stage=state_.connect_sent?CONNECTED:READY_TO_CONNECT;
        else if(strcmp(name,"WaitingForResources")==0 ||
                strcmp(name,"Queued")==0 || strcmp(name,"InQueue")==0) {
            state_.stage=QUEUED;
            const cJSON* queue=cJSON_GetObjectItemCaseSensitive(root,"queueDetails");
            const cJSON* wait=cJSON_GetObjectItemCaseSensitive(queue,"estimatedWaitTime");
            if(cJSON_IsNumber(wait) && wait->valuedouble>=0 &&
               wait->valuedouble<=86400 &&
               wait->valuedouble==(double)(unsigned int)wait->valuedouble)
                state_.queue_seconds=(unsigned int)wait->valuedouble;
        } else if(strcmp(name,"New")==0 || strcmp(name,"Provisioning")==0)
            state_.stage=PROVISIONING;
        else if(strcmp(name,"Failed")==0 || strcmp(name,"Terminated")==0 ||
                strcmp(name,"Canceled")==0)
            okay=fail("SESSION_SERVER_TERMINATED");
        else okay=fail("SESSION_STATE_UNSUPPORTED");
        if(okay && name) {
            strcpy_s(state_.server_state,sizeof(state_.server_state),name);
            if(state_.stage!=QUEUED)state_.queue_seconds=0;
        }
    }
    free_json(root);secure_http::secure_zero(response,sizeof(*response));
    delete response;return okay;
}
bool Session::connect(const char* token,const char* transfer) {
    if(!needs_cleanup() || state_.stage!=READY_TO_CONNECT)
        return fail("SESSION_NOT_READY_TO_CONNECT");
    if(state_.connect_sent)return fail("SESSION_CONNECT_ALREADY_SENT");
    if(!token_valid(transfer))return fail("SESSION_TRANSFER_REQUIRED");
    const size_t capacity=7200;
    char* body=new(std::nothrow) char[capacity];
    secure_http::Response* response=new(std::nothrow) secure_http::Response;
    if(!body || !response) {
        delete [] body;delete response;return fail("SESSION_NO_MEMORY");
    }
    const int n=sprintf_s(body,capacity,"{\"userToken\":\"%s\"}",transfer);
    bool okay=n>0 && (size_t)n<capacity;
    if(!okay)fail("SESSION_TRANSFER_INVALID");
    else okay=request("POST","/connect",token,body,(size_t)n,response);
    if(okay) {state_.connect_sent=true;state_.stage=CONNECTED;}
    secure_http::secure_zero(body,capacity);delete [] body;
    secure_http::secure_zero(response,sizeof(*response));delete response;
    return okay;
}
bool Session::stop(const char* token) {
    if(!needs_cleanup())return true;
    secure_http::Response* response=new(std::nothrow) secure_http::Response;
    if(!response)return fail("SESSION_NO_MEMORY");
    const Snapshot previous=state_;
    bool okay=request("DELETE","",token,NULL,0,response);
    state_.cleanup_http=state_.http_status;
    // The service may have expired/deleted our exact session already.
    if((state_.http_status==404 || state_.http_status==410) &&
       response->peer_certificate_verified && response->hostname_verified)
        okay=true;
    if(okay) {
        secure_http::secure_zero(path_,sizeof(path_));
        state_.cleanup_pending=false;
        if(previous.stage==FAILED) {
            state_.stage=FAILED;state_.http_status=previous.http_status;
            strcpy_s(state_.error_code,sizeof(state_.error_code),previous.error_code);
        } else {state_.stage=STOPPED;state_.error_code[0]=0;}
    }
    secure_http::secure_zero(response,sizeof(*response));delete response;
    return okay;
}
bool Session::needs_cleanup() const {return path_[0]!=0 && state_.cleanup_pending;}
const Snapshot& Session::snapshot() const {return state_;}
const char* stage_text(Stage stage) {
    switch(stage) {
    case IDLE:return "SESSION NOT STARTED";
    case CREATING:return "REQUESTING SESSION";
    case PROVISIONING:return "PREPARING SESSION";
    case QUEUED:return "WAITING IN QUEUE";
    case READY_TO_CONNECT:return "TRANSFER TOKEN REQUIRED";
    case CONNECTED:return "SESSION CONNECT ACCEPTED";
    case PROVISIONED:return "SESSION PROVISIONED";
    case STOPPED:return "SESSION CLOSED";
    default:return "SESSION FAILED";
    }
}
bool offline_selftest() {
    const char* host="brs.gssv-play-prod.xboxlive.com";
    const char* id="/v5/sessions/cloud/9F8AC269-11AA-22BB-33CC-445566778899";
    char normalized[64];
    if(!normalize_session_path(id+1,normalized,sizeof(normalized)) ||
       strcmp(normalized,id)!=0 || !validate_request("DELETE",host,id) ||
       validate_request("GET",host,id) ||
       validate_request("POST","evil.example",PLAY) ||
       normalize_session_path("/v5/sessions/cloud/../play",normalized,sizeof(normalized)))
        return false;
    Session no_backend;
    return !no_backend.start(host,"synthetic-gs-token-1234") &&
           !no_backend.needs_cleanup() &&
           strcmp(no_backend.snapshot().error_code,"SESSION_HTTPS_UNAVAILABLE")==0;
}
}
