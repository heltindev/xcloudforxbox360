#include "oauth_device.h"
#include "http_wire.h"
#include "json_fields.h"
#include <string.h>

namespace oauth_device {
namespace {
bool uri_is_https(const char* uri) {
    if(strncmp(uri,"https://",8)!=0)return false;
    const size_t length=strlen(uri);
    if(length<=8 || length>255)return false;
    for(size_t i=8;i<length;++i) {
        if((unsigned char)uri[i]<0x21 || uri[i]=='"' || uri[i]=='\\')
            return false;
    }
    return true;
}
void erase(void* pointer,size_t bytes) {
    volatile unsigned char* data=(volatile unsigned char*)pointer;
    while(bytes--) *data++=0;
}
}
bool make_device_form(const char* client_id,const char* scope,
                      char* output,size_t capacity) {
    if(!client_id||!scope||!output||capacity<48)return false;
    char encodedClient[256], encodedScope[512];
    if(!http_wire::form_escape(client_id,encodedClient,sizeof(encodedClient))||
       !http_wire::form_escape(scope,encodedScope,sizeof(encodedScope)))
        return false;
    const size_t a=strlen(encodedClient),b=strlen(encodedScope);
    const char prefix[]="client_id=";
    const char middle[]="&scope=";
    const size_t bytes=sizeof(prefix)-1+a+sizeof(middle)-1+b;
    if(bytes+1>capacity)return false;
    size_t offset=0;
    memcpy(output+offset,prefix,sizeof(prefix)-1);offset+=sizeof(prefix)-1;
    memcpy(output+offset,encodedClient,a);offset+=a;
    memcpy(output+offset,middle,sizeof(middle)-1);offset+=sizeof(middle)-1;
    memcpy(output+offset,encodedScope,b);offset+=b;
    output[offset]=0;
    return true;
}
bool read_device_response(const char* json,size_t length,
                          DeviceAuthorization* result) {
    if(!json||!result)return false;
    DeviceAuthorization pending;
    memset(&pending,0,sizeof(pending));
    const bool okay =
        json_fields::string(json,length,"verification_uri",
                            pending.verification_uri,sizeof(pending.verification_uri)) &&
        json_fields::string(json,length,"user_code",
                            pending.user_code,sizeof(pending.user_code)) &&
        json_fields::string(json,length,"device_code",
                            pending.device_code,sizeof(pending.device_code)) &&
        json_fields::uint_value(json,length,"expires_in",&pending.expires_in) &&
        uri_is_https(pending.verification_uri) &&
        pending.user_code[0]!=0 && pending.device_code[0]!=0 &&
        pending.expires_in>=60 && pending.expires_in<=86400;
    if(!okay) {
        clear(&pending);
        return false;
    }
    if(!json_fields::uint_value(json,length,"interval",&pending.interval))
        pending.interval=5;
    if(pending.interval<5)pending.interval=5;
    if(pending.interval>120)pending.interval=120;
    *result=pending;
    clear(&pending);
    return true;
}
void clear(DeviceAuthorization* auth) {
    if(auth)erase(auth,sizeof(*auth));
}
}
