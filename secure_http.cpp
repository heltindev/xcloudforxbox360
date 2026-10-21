#include "secure_http.h"
#include <string.h>

namespace secure_http {
void secure_zero(void* data,size_t n) {
    volatile unsigned char* p=(volatile unsigned char*)data;
    if(!p)return;
    while(n--)*p++=0;
}
Transport::Transport():callback_(NULL),context_(NULL) {}
void Transport::attach(FormPost f,void* ctx) {
    callback_=f;
    context_=f?ctx:NULL;
}
bool Transport::configured() const {return callback_!=NULL;}
bool Transport::post_form(const char* host,const char* path,
                          const char* body,size_t count,Response* out) const {
    if(!out)return false;
    memset(out,0,sizeof(*out));
    if(!callback_||!host||!path||!body||count==0||count>8192)
        return false;
    // The initial Device Code implementation talks to this Microsoft
    // hostname only. No arbitrary URLs or user supplied credentials.
    if(strcmp(host,"login.microsoftonline.com")!=0)return false;
    const char* allowed[]={
        "/consumers/oauth2/v2.0/devicecode",
        "/consumers/oauth2/v2.0/token"
    };
    if(strcmp(path,allowed[0])!=0 && strcmp(path,allowed[1])!=0)
        return false;
    if(!callback_(context_,host,path,body,count,out)) {
        secure_zero(out,sizeof(*out));
        return false;
    }
    if(!out->peer_certificate_verified || !out->hostname_verified ||
       out->length==0 || out->length>=RESPONSE_CAP ||
       out->http_status<100 || out->http_status>=600) {
        secure_zero(out,sizeof(*out));
        return false;
    }
    out->body[out->length]=0;
    return true;
}
}
