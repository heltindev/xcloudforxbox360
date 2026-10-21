#include "xcloud_session.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return false;}}while(0)
using namespace xcloud_session;
namespace {
const char HOST[]="brs.gssv-play-prod.xboxlive.com";
const char TOKEN[]="synthetic-gs-token-1234";
const char ID_PATH[]="/v5/sessions/cloud/9F8AC269-11AA-22BB-33CC-445566778899";
struct Fixture {unsigned int step;int mode;unsigned int deletes;bool connect_seen;};
bool reply(void* context,const char* method,const char* host,const char* path,
           const char* token,const char* body,size_t bytes,secure_http::Response* out) {
    Fixture* f=(Fixture*)context;
    CHECK(strcmp(host,HOST)==0 && strcmp(token,TOKEN)==0);
    CHECK(validate_request(method,host,path));
    memset(out,0,sizeof(*out));out->http_status=200;
    out->peer_certificate_verified=true;out->hostname_verified=true;
    const char* json="{}";
    if(strcmp(method,"DELETE")==0) {
        CHECK(strcmp(path,ID_PATH)==0 && !body && !bytes);
        ++f->deletes;out->http_status=f->mode==9?404:204;json="";
    } else if(strcmp(path,"/v5/sessions/cloud/play")==0) {
        CHECK(strcmp(method,"POST")==0 && body && bytes==strlen(body));
        CHECK(strstr(body,"\"titleId\":\"FORTNITE\"")!=NULL);
        CHECK(strstr(body,"systemUpdateGroup")==NULL);
        json="{\"sessionPath\":\"v5/sessions/cloud/9F8AC269-11AA-22BB-33CC-445566778899\",\"queueConfig\":{\"pollIntervalSeconds\":1}}";
        if(f->mode==1)json="{\"sessionPath\":\"https://evil.example/session\"}";
        if(f->mode==2) {out->http_status=403;json="{\"code\":\"EntitlementDenied\",\"message\":\"secret-token-value\"}";}
        if(f->mode==3)out->hostname_verified=false;
        if(f->mode==4)json="{\"sessionPath\":\"v5/sessions/cloud/9F8AC269-11AA-22BB-33CC-445566778899\",\"sessionPath\":\"different\"}";
    } else if(strstr(path,"/connect")) {
        CHECK(strcmp(method,"POST")==0 && body && bytes==strlen(body));
        CHECK(strcmp(body,"{\"userToken\":\"synthetic-transfer-token-1234\"}")==0);
        CHECK(!f->connect_seen);f->connect_seen=true;out->http_status=204;json="";
    } else {
        CHECK(strcmp(method,"GET")==0 && !body && !bytes);
        if(f->mode==5)json="{\"state\":\"Provisioning\"}trailing";
        else if(f->mode==6)json="{\"state\":\"Failed\",\"errorDetails\":{\"message\":\"secret-token-value\"}}";
        else if(f->mode==7) {out->http_status=401;json="{\"code\":\"TokenExpired\"}";}
        else if(f->mode==8)json="{\"state\":\"WaitingForResources\"}";
        else if(f->step==1)json="{\"state\":\"WaitingForResources\",\"queueDetails\":{\"estimatedWaitTime\":42,\"position\":3},\"errorDetails\":null}";
        else if(!f->connect_seen)json="{\"state\":\"ReadyToConnect\",\"errorDetails\":null}";
        else json="{\"state\":\"Provisioned\",\"errorDetails\":null}";
    }
    strcpy_s(out->body,sizeof(out->body),json);out->length=strlen(json);
    ++f->step;return true;
}
bool lifecycle() {
    Fixture f={0,0,0,false};Session s;s.attach(reply,&f);
    CHECK(s.start(HOST,TOKEN));CHECK(s.needs_cleanup());
    CHECK(s.snapshot().stage==PROVISIONING && s.snapshot().session_created);
    CHECK(s.poll(TOKEN) && s.snapshot().stage==QUEUED);
    CHECK(s.snapshot().queue_seconds==42);
    CHECK(s.poll(TOKEN) && s.snapshot().stage==READY_TO_CONNECT);
    CHECK(s.connect(TOKEN,"synthetic-transfer-token-1234"));
    CHECK(s.snapshot().connect_sent && s.snapshot().stage==CONNECTED);
    CHECK(s.poll(TOKEN) && s.snapshot().stage==PROVISIONED);
    CHECK(s.stop(TOKEN) && f.deletes==1 && !s.needs_cleanup());
    CHECK(s.snapshot().stage==STOPPED);
    CHECK(s.stop(TOKEN) && f.deletes==1);return true;
}
bool routes() {
    char path[64];
    CHECK(normalize_session_path(ID_PATH,path,sizeof(path)) && strcmp(path,ID_PATH)==0);
    CHECK(normalize_session_path(ID_PATH+1,path,sizeof(path)) && strcmp(path,ID_PATH)==0);
    const char* bad[]={"/v5/sessions/cloud/abc","//v5/sessions/cloud/9F8AC269-11AA-22BB-33CC-445566778899",
        "/v5/sessions/cloud/9F8AC269-11AA-22BB-33CC-445566778899/../play",
        "/v5/sessions/cloud/9F8AC269-11AA-22BB-33CC-445566778899?token=secret",
        "https://brs.gssv-play-prod.xboxlive.com/v5/sessions/cloud/9F8AC269-11AA-22BB-33CC-445566778899"};
    for(unsigned int i=0;i<sizeof(bad)/sizeof(bad[0]);++i)
        CHECK(!normalize_session_path(bad[i],path,sizeof(path)));
    CHECK(validate_request("DELETE",HOST,ID_PATH));
    CHECK(!validate_request("GET",HOST,ID_PATH));
    CHECK(!validate_request("DELETE",HOST,"/v5/sessions/cloud/active"));
    CHECK(!validate_request("POST","evil.example","/v5/sessions/cloud/play"));
    CHECK(!validate_request("POST","xboxlive.com.evil.example","/v5/sessions/cloud/play"));
    CHECK(!validate_request("POST","@brs.gssv-play-prod.xboxlive.com","/v5/sessions/cloud/play"));
    CHECK(!validate_request("POST","brs.gssv-play-prod.xboxlive.com\r\nX: y","/v5/sessions/cloud/play"));
    CHECK(!validate_request("POST","brs..gssv-play-prod.xboxlive.com","/v5/sessions/cloud/play"));
    CHECK(!validate_request("POST","-brs.gssv-play-prod.xboxlive.com","/v5/sessions/cloud/play"));
    return true;
}
bool failures() {
    for(int mode=1;mode<=4;++mode) {
        Fixture f={0,mode,0,false};Session s;s.attach(reply,&f);
        CHECK(!s.start(HOST,TOKEN));CHECK(!s.needs_cleanup());
        CHECK(s.snapshot().stage==FAILED);
        CHECK(strstr(s.snapshot().error_code,"secret")==NULL);
        if(mode==2)CHECK(strcmp(s.snapshot().error_code,"EntitlementDenied")==0);
    }
    for(int mode=5;mode<=7;++mode) {
        Fixture f={0,mode,0,false};Session s;s.attach(reply,&f);
        CHECK(s.start(HOST,TOKEN));CHECK(!s.poll(TOKEN));
        CHECK(s.needs_cleanup() && s.snapshot().stage==FAILED);
        CHECK(strstr(s.snapshot().error_code,"secret")==NULL);
        CHECK(s.stop(TOKEN) && f.deletes==1 && !s.needs_cleanup());
        CHECK(s.snapshot().stage==FAILED && s.snapshot().cleanup_http==204);
    }
    Fixture f={0,8,0,false};Session s;s.attach(reply,&f);
    CHECK(s.start(HOST,TOKEN));
    for(unsigned int i=0;i<MAX_STATE_POLLS;++i)CHECK(s.poll(TOKEN));
    const unsigned int before=f.step;
    CHECK(!s.poll(TOKEN) && f.step==before);
    CHECK(strcmp(s.snapshot().error_code,"SESSION_WAIT_LIMIT")==0);
    CHECK(s.stop(TOKEN));
    Fixture g={0,9,0,false};Session expired;expired.attach(reply,&g);
    CHECK(expired.start(HOST,TOKEN) && expired.stop(TOKEN));
    CHECK(!expired.needs_cleanup() && expired.snapshot().cleanup_http==404);
    Fixture h={0,0,0,false};Session invalid;invalid.attach(reply,&h);
    CHECK(!invalid.start(HOST,"bad\r\nAuthorization: secret") && !h.step);
    CHECK(!invalid.start("evil.example",TOKEN) && !h.step);
    CHECK(invalid.start(HOST,TOKEN));CHECK(!invalid.start(HOST,TOKEN));
    CHECK(h.step==1 && invalid.needs_cleanup());CHECK(invalid.stop(TOKEN));
    return true;
}
}
int main() {
    if(!offline_selftest()||!routes()||!lifecycle()||!failures())return 1;
    puts("PASS: session routes, queue, transfer, provision, cleanup, TLS rejection and bounded retries.");
    return 0;
}
