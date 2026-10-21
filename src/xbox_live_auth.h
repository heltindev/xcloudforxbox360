#pragma once
#include <stddef.h>

// Offline Xbox Live / XSTS authorization request builders.
// They DO NOT perform HTTP, exchange tokens, or launch xCloud.
// Live calls require a permitted Microsoft-issued XboxLive.signin scope
// and verified HTTPS to the right service host.
namespace xbox_live_auth {
enum Gate {
    MICROSOFT_ACCOUNT_REQUIRED=0,
    XBOX_SCOPE_REQUIRED=1,
    XBOX_USER_TOKEN_REQUIRED=2,
    XSTS_TOKEN_REQUIRED=3,
    XCLOUD_SESSION_REQUIRED=4,
    WEBRTC_REQUIRED=5,
    VIDEO_DECODER_REQUIRED=6,
    AUDIO_DECODER_REQUIRED=7,
    CONTROLLER_CHANNEL_REQUIRED=8,
    STREAM_READY=9
};
struct Status {
    bool microsoft_oauth;
    bool xbox_scope;
    bool xbox_user_token;
    bool xsts_token;
    bool cloud_session;
    bool webrtc;
    bool video_decoder;
    bool audio_decoder;
    bool controller_channel;
};
// Scopes are a compile-time opt-in based on local auth_config.h.
const char* configured_scopes();
bool xbox_scope_request_enabled();
bool has_scope(const char* scopes,const char* exact_scope);
Gate first_blocker(const Status& status);
const char* gate_text(Gate gate);
// Format only; no network, no retained tokens, caller MUST erase output.
bool make_user_auth_request(const char* msa_access_token,
                            char* out,size_t capacity);
bool make_xsts_request(const char* xbox_user_token,
                       char* out,size_t capacity);
// Explicitly choose an allowed relying party; streaming is a DIFFERENT
// service from a generic Xbox Live XSTS token.
bool make_xsts_request_for_service(const char* xbox_user_token,
                                   const char* relying_party,
                                   char* out,size_t capacity);
bool offline_selftest();
}
