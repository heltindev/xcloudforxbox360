#include "xbox_live_exchange.h"
#include "http_wire.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return false;}}
using namespace xbox_live_exchange;
struct Fixture {unsigned int step,now,cancel_at;int mode;};
static unsigned int clock_ms(void* p){return ((Fixture*)p)->now;}
static bool cancelled(void* p){Fixture* f=(Fixture*)p;return f->step>=f->cancel_at;}
static bool post(void* p,const char*,const char*,const char*,size_t,secure_http::Response* out){
    Fixture* f=(Fixture*)p;
    memset(out,0,sizeof(*out));
    out->http_status=200;out->peer_certificate_verified=true;out->hostname_verified=true;
    const char* json;
    if(f->step<2)json="{\"Token\":\"test-server-token-1234\",\"DisplayClaims\":{\"xui\":[]}}";
    else{
        if(f->mode==1)out->http_status=403;
        if(f->mode==2)out->hostname_verified=false;
        json=f->mode==3?
        "{\"gsToken\":\"test-cloud-token-1234\",\"durationInSeconds\":0.5,\"offeringSettings\":{\"regions\":[{\"baseUri\":\"https://brs.gssv-play-prod.xboxlive.com\"}]}}":
        "{\"gsToken\":\"test-cloud-token-1234\",\"durationInSeconds\":3600,\"offeringSettings\":{\"regions\":[{\"baseUri\":\"https://east.gssv-play-prod.xboxlive.com\"},{\"baseUri\":\"https://brs.gssv-play-prod.xboxlive.com\",\"isDefault\":true}]}}";
    }
    strcpy_s(out->body,sizeof(out->body),json);out->length=strlen(json);++f->step;return true;
}
static bool credentials_test(){
    CloudCredentials c;
    unsigned int tick=0xffffff00u;
    const char* uri="https://brs.gssv-play-prod.xboxlive.com";
    CHECK(!c.token_for_worker(0));
    CHECK(c.retain("test-cloud-token-1234",uri,2,tick));
    const char* secret=c.token_for_worker(tick);
    CHECK(secret && c.remaining_seconds(tick)==2);
    CHECK(c.remaining_seconds(tick+1999u)==1);
    CHECK(!c.token_for_worker(tick+2000u));
    for(unsigned int n=0;n<20;++n)CHECK(secret[n]==0);
    CHECK(!c.host_for_worker(tick+2000u));
    CHECK(!c.retain("test-cloud-token-1234","https://evil.example",2,10));
    CHECK(!c.retain("test-cloud-token-1234",uri,0,10));
    CHECK(!c.retain("test-cloud-token\r\ninjected",uri,2,10));
    CHECK(c.retain("test-cloud-token-1234",uri,2147483647u,0));
    CHECK(c.remaining_seconds(0)==86400u);
    CHECK(!c.token_for_worker(86400000u));
    Fixture f={0,1000,100,0};WorkerContext w={&c,clock_ms,cancelled,&f};Result result;
    CHECK(exchange("sample-msa-token-123",true,post,&f,&result,&w));
    CHECK(result.cloud_login_received && result.cloud_regions==2 && f.step==3);
    CHECK(c.token_for_worker(f.now) && strcmp(c.token_for_worker(f.now),"test-cloud-token-1234")==0);
    CHECK(strcmp(c.host_for_worker(f.now),"brs.gssv-play-prod.xboxlive.com")==0);
    CHECK(c.remaining_seconds(f.now)==3600);
    for(unsigned int at=0;at<=3;++at){
        f.step=0;f.cancel_at=at;
        CHECK(!exchange("sample-msa-token-123",true,post,&f,&result,&w));
        CHECK(result.stage==EXCHANGE_CANCELLED && f.step==at);
        CHECK(!c.token_for_worker(f.now));
    }
    for(int mode=1;mode<=3;++mode){
        f.step=0;f.mode=mode;f.cancel_at=100;
        CHECK(c.retain("previous-cloud-token-123",uri,3600,f.now));
        CHECK(!exchange("sample-msa-token-123",true,post,&f,&result,&w));
        CHECK(!c.token_for_worker(f.now));
    }
    CHECK(c.retain("previous-cloud-token-123",uri,3600,f.now));
    CHECK(!exchange("sample-msa-token-123",false,post,&f,&result,&w));
    CHECK(!c.token_for_worker(f.now));return true;
}
static bool framing_test(){
    const char* messages[]={
        "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: keep-alive\r\n\r\n{}",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: Chunked\r\n\r\n1\r\n{\r\n1\r\n}\r\n0\r\n\r\n"};
    for(unsigned int n=0;n<2;++n){
        const size_t bytes=strlen(messages[n]);
        for(size_t i=0;i<bytes;++i)CHECK(!http_wire::response_complete(messages[n],i));
        CHECK(http_wire::response_complete(messages[n],bytes));
    }
    char output[8];size_t decoded=0;
    const char payload[]="1\r\n{\r\n1\r\n}\r\n0\r\n\r\n";
    CHECK(http_wire::decode_chunked(payload,sizeof(payload)-1,output,sizeof(output),&decoded));
    CHECK(decoded==2 && memcmp(output,"{}",2)==0);
    CHECK(!http_wire::decode_chunked(payload,sizeof(payload)-1,output,1,&decoded));
    const char* bad_chunks[]={"0\r\n\r\njunk","f\r\nx\r\n0\r\n\r\n","1;ext\r\nx\r\n0\r\n\r\n","FFFFFFFFF\r\n"};
    for(unsigned int n=0;n<4;++n)CHECK(!http_wire::decode_chunked(bad_chunks[n],strlen(bad_chunks[n]),output,sizeof(output),&decoded));
    const char* bad[]={
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\n{}",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunkedJUNK\r\n\r\n",
        "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nTransfer-Encoding: chunked\r\n\r\n",
        "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Length: 2\r\n\r\n{}",
        "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nTransfer-Encoding: chunked\r\n\r\n{}"};
    http_wire::Response parsed;
    for(unsigned int i=0;i<5;++i)CHECK(!http_wire::parse_response(bad[i],strlen(bad[i]),&parsed));
    const char close_delimited[]="HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n{}";
    CHECK(!http_wire::response_complete(close_delimited,sizeof(close_delimited)-1));
    char request[1024];size_t bytes;
    const char body[]="{\"token\":\"test-cloud-token-1234\",\"offeringId\":\"xgpuweb\"}";
    CHECK(http_wire::make_cloud_login_request(body,sizeof(body)-1,request,sizeof(request),&bytes));
    CHECK(strstr(request,"x-gssv-client: XboxComBrowser\r\n") && bytes==strlen(request));
    CHECK(!http_wire::make_cloud_login_request(body,sizeof(body)-1,request,64,&bytes));return true;
}
int main(){
    if(!xbox_live_exchange::offline_selftest() || !credentials_test() || !framing_test())return 1;
    puts("PASS: auth; credential expiry/wrap/clear/default region; cancellation; HTTP fragmentation/chunks/ambiguity.");
    return 0;
}
