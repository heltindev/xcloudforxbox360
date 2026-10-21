#include "login_cache.h"
#include "secure_http.h"
#include <stdlib.h>
#include <new>
#include <string.h>
namespace login_cache {
namespace {
const unsigned char MAGIC[8]={'X','3','6','L','O','G','0','1'};
bool text(const char* p,size_t cap,bool allow_empty) {
    if(!p)return false;
    size_t n=0;
    for(;n<cap && p[n];++n)
        if((unsigned char)p[n]<0x20 || (unsigned char)p[n]>0x7e)return false;
    return n<cap && (allow_empty || n>0);
}
bool client_ok(const char* p) {
    if(!p || strlen(p)!=36)return false;
    for(unsigned int i=0;i<36;++i) {
        const char c=p[i];
        if(i==8||i==13||i==18||i==23) {if(c!='-')return false;}
        else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;
    }
    return true;
}
unsigned int be16(const unsigned char* p) {return ((unsigned int)p[0]<<8)|p[1];}
void put16(unsigned char* p,unsigned int n) {p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
struct Workspace {
    unsigned char plain[PLAIN_CAP],check[PLAIN_CAP],blob[BLOB_CAP];
    ~Workspace(){secure_http::secure_zero(this,sizeof(*this));}
};
class Scratch {
public:
    Scratch():p(new(std::nothrow) Workspace) {if(p)memset(p,0,sizeof(*p));}
    ~Scratch(){delete p;}
    Workspace* p;
private:
    Scratch(const Scratch&);Scratch& operator=(const Scratch&);
};
}
SavedLogin::SavedLogin(){clear();}
SavedLogin::~SavedLogin(){clear();}
void SavedLogin::clear(){secure_http::secure_zero(refresh,sizeof(refresh));secure_http::secure_zero(granted,sizeof(granted));}
Status Store::save(const char* client,const char* requested,const char* granted,const char* refresh) {
    if(!client_ok(client)||!text(requested,SCOPE_CAP,false)||
       !text(granted,SCOPE_CAP,true)||!text(refresh,TOKEN_CAP,false))return INVALID;
    Scratch s;if(!s.p)return IO_ERROR;
    const unsigned int rq=(unsigned int)strlen(requested),gr=(unsigned int)strlen(granted),
                       rt=(unsigned int)strlen(refresh);
    const size_t count=50u+rq+gr+rt;
    memcpy(s.p->plain,MAGIC,8);memcpy(s.p->plain+8,client,36);
    put16(s.p->plain+44,rq);put16(s.p->plain+46,gr);put16(s.p->plain+48,rt);
    memcpy(s.p->plain+50,requested,rq);memcpy(s.p->plain+50+rq,granted,gr);
    memcpy(s.p->plain+50+rq+gr,refresh,rt);
    size_t bytes=0,checked=0;
    if(!backend_.protect(s.p->plain,count,s.p->blob,BLOB_CAP,&bytes) ||
       !bytes || bytes>BLOB_CAP ||
       !backend_.unprotect(s.p->blob,bytes,s.p->check,PLAIN_CAP,&checked) ||
       checked!=count || memcmp(s.p->plain,s.p->check,count)!=0)return PROTECTION_ERROR;
    return backend_.write_atomic(s.p->blob,bytes)?OK:IO_ERROR;
}
Status Store::load(const char* client,const char* requested,SavedLogin* out) {
    if(out)out->clear();
    if(!out||!client_ok(client)||!text(requested,SCOPE_CAP,false))return INVALID;
    Scratch s;if(!s.p)return IO_ERROR;
    size_t bytes=0,count=0;
    const Status read=backend_.read(s.p->blob,BLOB_CAP,&bytes);
    if(read!=OK)return read;
    if(!bytes||bytes>BLOB_CAP)return INVALID;
    if(!backend_.unprotect(s.p->blob,bytes,s.p->plain,PLAIN_CAP,&count))return PROTECTION_ERROR;
    if(count<50 || count>PLAIN_CAP || memcmp(s.p->plain,MAGIC,8)!=0)return INVALID;
    const unsigned int rq=be16(s.p->plain+44),gr=be16(s.p->plain+46),rt=be16(s.p->plain+48);
    if(!rq||rq>=SCOPE_CAP||gr>=SCOPE_CAP||!rt||rt>=TOKEN_CAP ||
       count!=50u+rq+gr+rt)return INVALID;
    if(memcmp(s.p->plain+8,client,36)!=0 || strlen(requested)!=rq ||
       memcmp(s.p->plain+50,requested,rq)!=0)return WRONG_APP;
    // Reject embedded NUL and control characters before exposing any secret.
    for(size_t i=50;i<count;++i)
        if(s.p->plain[i]<0x20 || s.p->plain[i]>0x7e)return INVALID;
    memcpy(out->granted,s.p->plain+50+rq,gr);out->granted[gr]=0;
    memcpy(out->refresh,s.p->plain+50+rq+gr,rt);out->refresh[rt]=0;
    return OK;
}
}
