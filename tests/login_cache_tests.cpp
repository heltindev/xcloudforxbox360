#include "login_cache.h"
#include "login_cache_platform.h"
#include "secure_http.h"
#include <stdio.h>
#include <string.h>
using namespace login_cache;
#define CHECK(x) {if(!(x)){printf("FAIL cache test line %d\n",__LINE__);return 1;}}
static const char CLIENT[]="00000000-0000-4000-8000-000000000001";
static const char OTHER[]="00000000-0000-4000-8000-000000000002";
static const char SCOPE[]="XboxLive.signin offline_access";
static const char TOKEN[]="synthetic-refresh-value/= &?%123456789";
class TestPlatform : public LoginCachePlatform {
public:
    TestPlatform():LoginCachePlatform("cache-test.authcache"),fail_write(false),fail_protect(false){}
    bool fail_write,fail_protect;
    virtual bool write_atomic(const unsigned char* p,size_t n) {
        return !fail_write && LoginCachePlatform::write_atomic(p,n);
    }
    virtual bool protect(const unsigned char* p,size_t n,unsigned char* out,size_t cap,size_t* written) {
        return !fail_protect && LoginCachePlatform::protect(p,n,out,cap,written);
    }
};
bool empty(const SavedLogin& s) {
    for(unsigned int i=0;i<TOKEN_CAP;++i)if(s.refresh[i])return false;
    for(unsigned int i=0;i<SCOPE_CAP;++i)if(s.granted[i])return false;
    return true;
}
int main() {
    TestPlatform backend;Store store(backend);SavedLogin loaded;
    CHECK(store.erase());
    CHECK(store.load(CLIENT,SCOPE,&loaded)==MISSING && empty(loaded));
    CHECK(store.save(CLIENT,SCOPE,SCOPE,TOKEN)==OK);
    CHECK(store.load(CLIENT,SCOPE,&loaded)==OK);
    CHECK(strcmp(loaded.refresh,TOKEN)==0 && strcmp(loaded.granted,SCOPE)==0);
    unsigned char blob[BLOB_CAP];size_t bytes=0;
    CHECK(backend.read(blob,sizeof(blob),&bytes)==OK && bytes>0);
    bool plaintext=false;
    for(size_t i=0;i+strlen(TOKEN)<=bytes;++i)
        if(memcmp(blob+i,TOKEN,strlen(TOKEN))==0)plaintext=true;
    CHECK(!plaintext); // Uses real Windows DPAPI in this host-only suite.
    CHECK(store.load(OTHER,SCOPE,&loaded)==WRONG_APP && empty(loaded));
    CHECK(store.load(CLIENT,"offline_access",&loaded)==WRONG_APP && empty(loaded));
    backend.fail_write=true;
    CHECK(store.save(CLIENT,SCOPE,SCOPE,"replacement-refresh-123")==IO_ERROR);
    backend.fail_write=false;
    CHECK(store.load(CLIENT,SCOPE,&loaded)==OK && strcmp(loaded.refresh,TOKEN)==0);
    backend.fail_protect=true;
    CHECK(store.save(CLIENT,SCOPE,SCOPE,"replacement-refresh-123")==PROTECTION_ERROR);
    backend.fail_protect=false;
    CHECK(store.load(CLIENT,SCOPE,&loaded)==OK && strcmp(loaded.refresh,TOKEN)==0);
    blob[bytes-1]^=0x80;
    CHECK(backend.write_atomic(blob,bytes));
    CHECK(store.load(CLIENT,SCOPE,&loaded)!=OK && empty(loaded));
    CHECK(store.save(CLIENT,SCOPE,SCOPE,TOKEN)==OK);
    CHECK(backend.read(blob,sizeof(blob),&bytes)==OK && bytes>8);
    CHECK(backend.write_atomic(blob,bytes/2));
    CHECK(store.load(CLIENT,SCOPE,&loaded)!=OK && empty(loaded));
    unsigned char malformed[50]={0},protected_bad[BLOB_CAP];size_t bad_bytes=0;
    memcpy(malformed,"X36LOG01",8);memcpy(malformed+8,CLIENT,36);
    malformed[44]=0xff;malformed[45]=0xff;
    CHECK(backend.LoginCachePlatform::protect(malformed,sizeof(malformed),protected_bad,sizeof(protected_bad),&bad_bytes));
    CHECK(backend.write_atomic(protected_bad,bad_bytes));
    CHECK(store.load(CLIENT,SCOPE,&loaded)==INVALID && empty(loaded));
    CHECK(store.save(CLIENT,SCOPE,SCOPE,"bad\nrefresh")==INVALID);
    CHECK(store.save("invalid-client",SCOPE,SCOPE,TOKEN)==INVALID);
    CHECK(store.erase());
    CHECK(store.load(CLIENT,SCOPE,&loaded)==MISSING);
    secure_http::secure_zero(blob,sizeof(blob));secure_http::secure_zero(protected_bad,sizeof(protected_bad));
    puts("PASS: DPAPI-protected cache roundtrip, client/scope binding, rotation, tampering, truncation, atomic write failure, fail-closed protection and local erase. Native Xbox protection not executed.");
    return 0;
}
