#include "login_cache_platform.h"
#include "secure_http.h"
#if defined(_XBOX)
#include <xtl.h>
#include "xkelib.h"
#else
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib,"crypt32.lib")
#endif
#include <string.h>
LoginCachePlatform::LoginCachePlatform(const char* p) {
    memset(path_,0,sizeof(path_));memset(temporary_,0,sizeof(temporary_));
    if(!p)p="game:\\XCloud360.authcache";
    if(strlen(p)>=sizeof(path_)-5)return;
    strcpy_s(path_,sizeof(path_),p);strcpy_s(temporary_,sizeof(temporary_),p);
    strcat_s(temporary_,sizeof(temporary_),".tmp");
}
login_cache::Status LoginCachePlatform::read(unsigned char* out,size_t cap,size_t* bytes) {
    if(bytes)*bytes=0;
    if(!path_[0]||!out||!bytes||cap>0xffffffffu)return login_cache::IO_ERROR;
    HANDLE file=CreateFileA(path_,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE) {
        const DWORD e=GetLastError();
        return e==ERROR_FILE_NOT_FOUND || e==ERROR_PATH_NOT_FOUND?login_cache::MISSING:login_cache::IO_ERROR;
    }
    DWORD high=0;const DWORD size=GetFileSize(file,&high);
    if(high || size==INVALID_FILE_SIZE || !size || size>cap) {CloseHandle(file);return login_cache::INVALID;}
    DWORD got=0;const BOOL ok=ReadFile(file,out,size,&got,NULL);
    CloseHandle(file);
    if(!ok||got!=size){secure_http::secure_zero(out,cap);return login_cache::IO_ERROR;}
    *bytes=got;return login_cache::OK;
}
bool LoginCachePlatform::write_atomic(const unsigned char* data,size_t count) {
    if(!path_[0]||!data||!count||count>login_cache::BLOB_CAP)return false;
    HANDLE file=CreateFileA(temporary_,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_HIDDEN,NULL);
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;
    const bool okay=WriteFile(file,data,(DWORD)count,&written,NULL)!=FALSE &&
        written==count && FlushFileBuffers(file)!=FALSE;
    CloseHandle(file);
    if(!okay) {DeleteFileA(temporary_);return false;}
    if(!MoveFileExA(temporary_,path_,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temporary_);return false;
    }
    return true;
}
bool LoginCachePlatform::erase() {
    if(!path_[0])return false;
    const BOOL deleted=DeleteFileA(path_);
    const DWORD error=deleted?ERROR_SUCCESS:GetLastError();
    const BOOL temp_deleted=DeleteFileA(temporary_);
    const DWORD temp_error=temp_deleted?ERROR_SUCCESS:GetLastError();
    return (deleted||error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND) &&
           (temp_deleted||temp_error==ERROR_FILE_NOT_FOUND||temp_error==ERROR_PATH_NOT_FOUND);
}
bool LoginCachePlatform::protect(const unsigned char* plain,size_t count,unsigned char* out,size_t cap,size_t* written) {
    if(written)*written=0;
    if(!plain||!out||!written||count>login_cache::PLAIN_CAP||cap>0xffffffffu)return false;
#if defined(_XBOX)
    DWORD bytes=(DWORD)cap;
    if(!XeKeysObfuscate(XEKEY_OBFUSCATE_CONSOLE,(PBYTE)plain,(DWORD)count,out,&bytes) ||
       bytes==0 || bytes>cap)return false;
    *written=bytes;return true;
#else
    DATA_BLOB input={(DWORD)count,(BYTE*)plain},output={0,NULL};
    if(!CryptProtectData(&input,NULL,NULL,NULL,NULL,CRYPTPROTECT_UI_FORBIDDEN,&output))return false;
    const bool okay=output.cbData>0 && output.cbData<=cap;
    if(okay){memcpy(out,output.pbData,output.cbData);*written=output.cbData;}
    secure_http::secure_zero(output.pbData,output.cbData);LocalFree(output.pbData);
    return okay;
#endif
}
bool LoginCachePlatform::unprotect(const unsigned char* blob,size_t count,unsigned char* out,size_t cap,size_t* written) {
    if(written)*written=0;
    if(!blob||!out||!written||count>login_cache::BLOB_CAP||cap>0xffffffffu)return false;
#if defined(_XBOX)
    DWORD bytes=(DWORD)cap;
    if(!XeKeysUnObfuscate(XEKEY_OBFUSCATE_CONSOLE,(PBYTE)blob,(DWORD)count,out,&bytes) ||
       bytes==0 || bytes>cap) {secure_http::secure_zero(out,cap);return false;}
    *written=bytes;return true;
#else
    DATA_BLOB input={(DWORD)count,(BYTE*)blob},output={0,NULL};
    if(!CryptUnprotectData(&input,NULL,NULL,NULL,NULL,CRYPTPROTECT_UI_FORBIDDEN,&output))return false;
    const bool okay=output.cbData>0 && output.cbData<=cap;
    if(okay){memcpy(out,output.pbData,output.cbData);*written=output.cbData;}
    secure_http::secure_zero(output.pbData,output.cbData);LocalFree(output.pbData);
    return okay;
#endif
}
