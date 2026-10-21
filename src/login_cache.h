#pragma once
#include <stddef.h>
namespace login_cache {
enum { TOKEN_CAP=8192, SCOPE_CAP=256, PLAIN_CAP=9000, BLOB_CAP=16384 };
enum Status { OK=0, MISSING=1, IO_ERROR=2, INVALID=3, WRONG_APP=4, PROTECTION_ERROR=5 };
struct SavedLogin {
    char refresh[TOKEN_CAP];
    char granted[SCOPE_CAP];
    SavedLogin();
    ~SavedLogin();
    void clear();
private:
    SavedLogin(const SavedLogin&);
    SavedLogin& operator=(const SavedLogin&);
};
class Backend {
public:
    virtual ~Backend() {}
    virtual Status read(unsigned char* out,size_t cap,size_t* bytes)=0;
    virtual bool write_atomic(const unsigned char* data,size_t bytes)=0;
    virtual bool erase()=0;
    virtual bool protect(const unsigned char* plain,size_t bytes,
                         unsigned char* out,size_t cap,size_t* written)=0;
    virtual bool unprotect(const unsigned char* blob,size_t bytes,
                           unsigned char* out,size_t cap,size_t* written)=0;
};
class Store {
public:
    explicit Store(Backend& backend):backend_(backend) {}
    Status load(const char* client,const char* requested,SavedLogin* out);
    Status save(const char* client,const char* requested,
                const char* granted,const char* refresh);
    bool erase() {return backend_.erase();}
private:
    Store(const Store&);
    Store& operator=(const Store&);
    Backend& backend_;
};
}
