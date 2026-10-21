#pragma once
#include "login_cache.h"
class LoginCachePlatform : public login_cache::Backend {
public:
    // Production uses the executable's game: mount. Host tests supply an
    // isolated path under obj/. The only data written is a protected blob.
    explicit LoginCachePlatform(const char* path=NULL);
    virtual login_cache::Status read(unsigned char*,size_t,size_t*);
    virtual bool write_atomic(const unsigned char*,size_t);
    virtual bool erase();
    virtual bool protect(const unsigned char*,size_t,unsigned char*,size_t,size_t*);
    virtual bool unprotect(const unsigned char*,size_t,unsigned char*,size_t,size_t*);
private:
    char path_[260],temporary_[264];
};
