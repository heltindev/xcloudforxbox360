#include "http_wire.h"
#include <string.h>
#include <stdio.h>

namespace http_wire {
namespace {
bool token_char(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
}
bool valid_host(const char* str) {
    if (!str || !*str) return false;
    size_t n=0;
    while (n<=253 && str[n]) ++n;
    if (n>253) return false;
    for (size_t i=0; i<n; ++i)
        if (!token_char((unsigned char)str[i]) || str[i]=='_') return false;
    return true;
}
bool valid_path(const char* str) {
    if (!str || str[0] != '/') return false;
    size_t i=0;
    for (; str[i] && i<=1024; ++i) {
        const unsigned char c = (unsigned char)str[i];
        if (c < 0x21 || c >= 0x7f || c=='#') return false;
    }
    return i<=1024 && str[i]==0;
}
bool same_key(const char* text, const char* expected, size_t len) {
    const size_t n = strlen(expected);
    if (n != len) return false;
    for (size_t i=0; i<n; ++i) {
        char a=text[i],b=expected[i];
        if (a>='A' && a<='Z') a=(char)(a-'A'+'a');
        if (b>='A' && b<='Z') b=(char)(b-'A'+'a');
        if (a!=b) return false;
    }
    return true;
}
bool append(char* out,size_t cap,size_t* written,const char* p,size_t n) {
    if (!p || !out || !written || *written>=cap ||
        n>=cap-*written) return false;
    memcpy(out+*written,p,n);*written+=n;out[*written]=0;
    return true;
}
}
bool make_request(const char* method,const char* host,const char* path,
                  const char* content_type,const char* body,size_t body_length,
                  char* out,size_t capacity,size_t* written) {
    if (!out || capacity<100 || !written || !valid_host(host) ||
        !valid_path(path) || !method) return false;
    if (strcmp(method,"GET")!=0 && strcmp(method,"HEAD")!=0 &&
        strcmp(method,"POST")!=0) return false;
    if (body_length>MAX_BODY || (body_length!=0 && !body) ||
        (!body && strcmp(method,"POST")==0 && body_length!=0))
        return false;
    if (content_type && strcmp(content_type,"application/x-www-form-urlencoded")!=0 &&
        strcmp(content_type,"application/json")!=0) return false;
    out[0]=0;size_t used=0;
    char sizeLine[48];
    if (!append(out,capacity,&used,method,strlen(method)) ||
        !append(out,capacity,&used," ",1) ||
        !append(out,capacity,&used,path,strlen(path)) ||
        !append(out,capacity,&used," HTTP/1.1\r\nHost: ",17) ||
        !append(out,capacity,&used,host,strlen(host)) ||
        !append(out,capacity,&used,
                "\r\nUser-Agent: XCloud360/0.10\r\nAccept: application/json\r\nConnection: close\r\n",
                strlen("\r\nUser-Agent: XCloud360/0.10\r\nAccept: application/json\r\nConnection: close\r\n")))
        return false;
    if (strcmp(method,"POST")==0) {
        const int printed=sprintf_s(sizeLine,sizeof(sizeLine),"%u",(unsigned int)body_length);
        if (printed<=0) return false;
        if (!append(out,capacity,&used,"Content-Length: ",16) ||
            !append(out,capacity,&used,sizeLine,(size_t)printed) ||
            !append(out,capacity,&used,"\r\nContent-Type: ",16) ||
            !append(out,capacity,&used,content_type?content_type:"application/json",
                    strlen(content_type?content_type:"application/json")) ||
            !append(out,capacity,&used,"\r\n",2))
            return false;
    }
    if (!append(out,capacity,&used,"\r\n",2) ||
        (body_length && !append(out,capacity,&used,body,body_length))) return false;
    *written=used;
    return true;
}
bool make_xbox_json_request(const char* host,const char* path,
                            const char* body,size_t body_length,
                            char* out,size_t capacity,size_t* written) {
    if(!out || !written || !body || !body_length)return false;
    if(!make_request("POST",host,path,"application/json",body,
                     body_length,out,capacity,written))return false;
    // The Xbox User and XSTS APIs require x-xbl-contract-version: 1.
    // Insert a fixed (not user-controlled) header in the existing bounds.
    static const char contract[]="x-xbl-contract-version: 1\r\n";
    const size_t n=sizeof(contract)-1;
    char* separator=strstr(out,"\r\n\r\n");
    if(!separator || *written+n>=capacity)return false;
    const size_t offset=(size_t)(separator-out)+2;
    // Keep the HTTP body and the final NUL, including embedded bytes
    // after the inserted header in the same request.
    memmove(out+offset+n,out+offset,*written-offset+1);
    memcpy(out+offset,contract,n);
    *written+=n;
    return true;
}
bool make_cloud_login_request(const char* body,size_t bytes,
                              char* out,size_t capacity,size_t* written) {
    if(!make_request("POST","xgpuweb.gssv-play-prod.xboxlive.com",
         "/v2/login/user","application/json",body,bytes,
         out,capacity,written))return false;
    static const char extra[]="x-gssv-client: XboxComBrowser\r\n";
    char* separator=strstr(out,"\r\n\r\n");
    const size_t n=sizeof(extra)-1;
    if(!separator || *written+n>=capacity)return false;
    const size_t at=(size_t)(separator-out)+2;
    memmove(out+at+n,out+at,*written-at+1);
    memcpy(out+at,extra,n);*written+=n;
    return true;
}
bool parse_response(const char* data,size_t length,Response* response) {
    if (!data || !response || length<16 || length>MAX_HEADERS+MAX_BODY) return false;
    size_t end=0;
    for (size_t i=0; i+3<length && i<MAX_HEADERS; ++i) {
        if (data[i]=='\r' && data[i+1]=='\n' &&
            data[i+2]=='\r' && data[i+3]=='\n') {end=i+4;break;}
    }
    if (!end || memcmp(data,"HTTP/1.",7)!=0 ||
        (data[7]!='0' && data[7]!='1') || data[8]!=' ' ||
        data[9]<'0' || data[9]>'9' ||
        data[10]<'0' || data[10]>'9' ||
        data[11]<'0' || data[11]>'9') return false;
    Response parsed;
    parsed.status=(data[9]-'0')*100+(data[10]-'0')*10+(data[11]-'0');
    parsed.body_offset=end;
    parsed.content_length=0;
    parsed.has_length=false;
    parsed.chunked=false;
    if (parsed.status<100 || parsed.status>599) return false;
    size_t at=0;
    while (at+1<end && !(data[at]=='\r' && data[at+1]=='\n')) ++at;
    at+=2;
    while (at+1<end && !(data[at]=='\r' && data[at+1]=='\n')) {
        size_t lineEnd=at;
        while (lineEnd+1<end && !(data[lineEnd]=='\r' && data[lineEnd+1]=='\n')) ++lineEnd;
        size_t colon=at;
        while (colon<lineEnd && data[colon]!=':') ++colon;
        if (colon>=lineEnd || colon==at) return false;
        size_t value=colon+1;
        while (value<lineEnd && (data[value]==' ' || data[value]=='\t')) ++value;
        if (same_key(data+at,"Content-Length",colon-at)) {
            if (parsed.has_length || value==lineEnd) return false;
            size_t n=0;
            for (size_t i=value;i<lineEnd;++i) {
                if (data[i]<'0' || data[i]>'9') return false;
                const size_t digit=(size_t)(data[i]-'0');
                if (n>(MAX_BODY-digit)/10) return false;
                n=n*10+digit;
            }
            parsed.content_length=n;
            parsed.has_length=true;
        }
        if (same_key(data+at,"Transfer-Encoding",colon-at)) {
            // Deliberately do not parse chunked body yet.
            if (lineEnd-value>=7 && memcmp(data+value,"chunked",7)==0)
                parsed.chunked=true;
        }
        at=lineEnd+2;
    }
    if (parsed.chunked && parsed.has_length) return false;
    *response=parsed;
    return true;
}
bool form_escape(const char* input,char* out,size_t cap) {
    if (!input || !out || cap==0) return false;
    static const char hex[]="0123456789ABCDEF";
    size_t used=0;
    for(size_t i=0;input[i];++i) {
        const unsigned char c=(unsigned char)input[i];
        if (token_char(c) || c=='~') {
            if (used+1>=cap) return false;
            out[used++]=(char)c;
        } else {
            if (used+3>=cap) return false;
            out[used++]='%';
            out[used++]=hex[c>>4];
            out[used++]=hex[c&15];
        }
    }
    out[used]=0;
    return true;
}
}
