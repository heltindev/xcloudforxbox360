#include "xbox_live_auth.h"
#include "../auth_config.h"
#include <string.h>
#include <stdio.h>

namespace xbox_live_auth {
bool xbox_scope_request_enabled() {
#if defined(XCLOUD360_ENABLE_XBOX_LIVE_SCOPE) && XCLOUD360_ENABLE_XBOX_LIVE_SCOPE
    return true;
#else
    return false;
#endif
}
const char* configured_scopes() {
    // OFF by default, preserving the known-working personal account flow.
    // Only turn on with an OWNED, legitimately permissioned app registration.
#if defined(XCLOUD360_ENABLE_XBOX_LIVE_SCOPE) && XCLOUD360_ENABLE_XBOX_LIVE_SCOPE
    // GR33N uses xboxlive.signin; keep OpenID scope for a clear
    // Microsoft v2 consent prompt. Never silently use another client ID.
    return "xboxlive.signin openid profile offline_access";
#else
    return "openid profile offline_access";
#endif
}
namespace {
bool token_char(unsigned char c) {
    return (c>='A' && c<='Z') || (c>='a' && c<='z') ||
           (c>='0' && c<='9') || c=='.' || c=='-' ||
           c=='_' || c=='~' || c=='+' || c=='/' || c=='=';
}
bool validate_token(const char* value,size_t maximum) {
    if(!value)return false;
    size_t n=0;
    while(value[n] && n<=maximum) {
        if(!token_char((unsigned char)value[n]))return false;
        ++n;
    }
    return n>=12 && n<=maximum && !value[n];
}
bool wrote(int n,size_t capacity) {
    return n>0 && (size_t)n<capacity;
}
}
bool has_scope(const char* scopes,const char* exact_scope) {
    if(!scopes || !exact_scope || !*exact_scope)return false;
    const size_t length=strlen(exact_scope);
    if(length>128)return false;
    const char* p=scopes;
    while(*p) {
        while(*p==' ')++p;
        const char* first=p;
        while(*p && *p!=' ')++p;
        if((size_t)(p-first)==length &&
           memcmp(first,exact_scope,length)==0)return true;
    }
    return false;
}
Gate first_blocker(const Status& s) {
    if(!s.microsoft_oauth)return MICROSOFT_ACCOUNT_REQUIRED;
    if(!s.xbox_scope)return XBOX_SCOPE_REQUIRED;
    if(!s.xbox_user_token)return XBOX_USER_TOKEN_REQUIRED;
    if(!s.xsts_token)return XSTS_TOKEN_REQUIRED;
    if(!s.cloud_session)return XCLOUD_SESSION_REQUIRED;
    if(!s.webrtc)return WEBRTC_REQUIRED;
    if(!s.video_decoder)return VIDEO_DECODER_REQUIRED;
    if(!s.audio_decoder)return AUDIO_DECODER_REQUIRED;
    if(!s.controller_channel)return CONTROLLER_CHANNEL_REQUIRED;
    return STREAM_READY;
}
const char* gate_text(Gate gate) {
    switch(gate) {
    case MICROSOFT_ACCOUNT_REQUIRED:return "MICROSOFT SIGN-IN REQUIRED";
    case XBOX_SCOPE_REQUIRED:return "XBOX LIVE SIGN-IN SCOPE NOT VERIFIED";
    case XBOX_USER_TOKEN_REQUIRED:return "XBOX USER TOKEN NOT ACQUIRED";
    case XSTS_TOKEN_REQUIRED:return "XSTS TOKEN NOT ACQUIRED";
    case XCLOUD_SESSION_REQUIRED:return "XCLOUD SESSION NOT CREATED";
    case WEBRTC_REQUIRED:return "WEBRTC SESSION NOT NEGOTIATED";
    case VIDEO_DECODER_REQUIRED:return "H264 VIDEO DECODER NOT INSTALLED";
    case AUDIO_DECODER_REQUIRED:return "AUDIO DECODER NOT INSTALLED";
    case CONTROLLER_CHANNEL_REQUIRED:return "GAMEPAD RETURN CHANNEL MISSING";
    case STREAM_READY:return "ALL REQUIRED STREAM CONDITIONS CONFIRMED";
    default:return "INVALID AUTHORIZATION STATE";
    }
}
bool make_user_auth_request(const char* token,
                            char* out,size_t capacity) {
    if(!out || capacity<192)return false;
    out[0]=0;
    if(!validate_token(token,12288))return false;
    const int n=sprintf_s(out,capacity,
        "{\"Properties\":{\"AuthMethod\":\"RPS\","
        "\"SiteName\":\"user.auth.xboxlive.com\","
        "\"RpsTicket\":\"d=%s\"},"
        "\"RelyingParty\":\"http://auth.xboxlive.com\","
        "\"TokenType\":\"JWT\"}",token);
    if(!wrote(n,capacity)) {out[0]=0;return false;}
    return true;
}
bool make_xsts_request_for_service(const char* user_token,
                                   const char* relying_party,
                                   char* out,size_t capacity) {
    if(!out || capacity<160)return false;
    out[0]=0;
    if(!validate_token(user_token,12288) || !relying_party)return false;
    // Never accept an untrusted URI as a JSON interpolation.
    if(strcmp(relying_party,"http://xboxlive.com")!=0 &&
       strcmp(relying_party,"http://gssv.xboxlive.com/")!=0)
        return false;
    const int n=sprintf_s(out,capacity,
        "{\"Properties\":{\"SandboxId\":\"RETAIL\","
        "\"UserTokens\":[\"%s\"]},"
        "\"RelyingParty\":\"%s\","
        "\"TokenType\":\"JWT\"}",user_token,relying_party);
    if(!wrote(n,capacity)) {out[0]=0;return false;}
    return true;
}
bool make_xsts_request(const char* user_token,
                       char* out,size_t capacity) {
    return make_xsts_request_for_service(user_token,
              "http://xboxlive.com",out,capacity);
}
bool offline_selftest() {
    if(has_scope("openid profile offline_access","XboxLive.signin") ||
       has_scope("notXboxLive.signin","XboxLive.signin") ||
       has_scope("XboxLive.signin.fake","XboxLive.signin") ||
       !has_scope("openid XboxLive.signin XboxLive.offline_access",
                  "XboxLive.signin") ||
       !has_scope("openid xboxlive.signin offline_access",
                  "xboxlive.signin"))return false;
    Status state={0};
    if(first_blocker(state)!=MICROSOFT_ACCOUNT_REQUIRED)return false;
    state.microsoft_oauth=true;
    if(first_blocker(state)!=XBOX_SCOPE_REQUIRED)return false;
    state.xbox_scope=true;
    if(first_blocker(state)!=XBOX_USER_TOKEN_REQUIRED)return false;
    state.xbox_user_token=true;
    if(first_blocker(state)!=XSTS_TOKEN_REQUIRED)return false;
    state.xsts_token=true;
    if(first_blocker(state)!=XCLOUD_SESSION_REQUIRED)return false;
    state.cloud_session=true;
    if(first_blocker(state)!=WEBRTC_REQUIRED)return false;
    state.webrtc=true;
    state.video_decoder=true;
    state.audio_decoder=true;
    state.controller_channel=true;
    if(first_blocker(state)!=STREAM_READY)return false;
    char json[512];
    if(!make_user_auth_request("demo-token-1234",json,sizeof(json)) ||
       !strstr(json,"\"RpsTicket\":\"d=demo-token-1234\""))return false;
    if(!make_xsts_request("demo-user-token-1234",json,sizeof(json)) ||
       !strstr(json,"\"SandboxId\":\"RETAIL\""))return false;
    if(make_user_auth_request("short",json,sizeof(json)) ||
       make_user_auth_request("token-1234-\"injection",json,sizeof(json)) ||
       make_xsts_request("token-1234-\\escape",json,sizeof(json)))
        return false;
    return true;
}
}
