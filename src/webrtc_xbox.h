#pragma once
#include <stddef.h>
#include <stdint.h>
#include "controller_input.h"
#include "xcloud_input.h"
#include "h264_rtp_receiver.h"

// Xbox 360 WebRTC implementation for xCloud
// Adapts GR33N/libpeer patterns to Xbox 360 XDK with XboxTLS/mbedTLS
// No external dependencies beyond what's in the project

namespace webrtc_xbox {

// WebRTC connection states
enum State {
    IDLE = 0,
    INITIALIZING,
    CREATING_CONNECTION,
    GENERATING_OFFER,
    SETTING_REMOTE_ANSWER,
    GATHERING_ICE,
    DTLS_HANDSHAKE,
    SCTP_STARTUP,
    OPENING_CHANNELS,
    SENDING_HANDSHAKE,
    WAITING_HANDSHAKE_ACK,
    SENDING_CAPABILITIES,
    SESSION_READY,
    RUNNING,
    STOPPING,
    FAILED
};

// ICE connection states (matching libpeer)
enum IceState {
    ICE_NEW = 0,
    ICE_CHECKING,
    ICE_CONNECTED,
    ICE_COMPLETED,
    ICE_FAILED,
    ICE_DISCONNECTED,
    ICE_CLOSED
};

// Data channel SIDs (MUST match xCloud: even numbers for DTLS client)
enum ChannelSid {
    SID_CONTROL  = 0,
    SID_INPUT    = 2,
    SID_MESSAGE  = 4,
    SID_CHAT     = 6
};

// Stream configuration constants (match xCloud expectations)
static const unsigned int DEFAULT_WIDTH  = 1280;
static const unsigned int DEFAULT_HEIGHT = 720;
static const unsigned int DEFAULT_FPS    = 60;
static const unsigned int DEFAULT_KBPS   = 10000;

// SDP constants for xCloud compatibility
static const char FMTP_XCLOUD[] =
    "a=fmtp:102 level-asymmetry-allowed=0;packetization-mode=1;"
    "profile-level-id=42e01f;max-fs=3600;max-mbps=108000";

// Diagnostics snapshot
struct Diagnostics {
    State state;
    IceState ice_state;

    // Timing measurements (microseconds)
    uint32_t us_usleep_1000;
    uint32_t us_usleep_20000;
    uint32_t us_select_1000;
    uint32_t us_poll_1000;

    uint32_t ms_init;
    uint32_t ms_create;
    uint32_t ms_offer;
    uint32_t ms_dtls;
    uint32_t ms_sctp;

    // SDP validation
    uint32_t sdp_len;
    int sdp_has_video_102;
    int sdp_has_fmtp;
    int sdp_setup_active;
    int sdp_fingerprint;
    int sdp_recvonly;
    int sdp_remb;
    int sdp_host_candidate;
    int sdp_srflx_candidate;

    // Runtime stats
    uint32_t video_packets;
    uint32_t video_bytes;
    uint32_t audio_packets;
    uint32_t audio_bytes;
    uint32_t access_units;
    uint32_t nacks_sent;
    uint32_t keyframes_requested;
    uint32_t input_reports_sent;
    uint32_t frames_decoded;
    uint32_t frames_dropped;
    uint32_t decode_time_avg_us;
    uint32_t audio_underruns;
    uint32_t audio_plc_frames;

    // ICE timing
    uint32_t ice_check_ms;
    uint32_t rtt_min_us;
    uint32_t rtt_avg_us;
    uint32_t rtt_max_us;
    uint32_t jitter_us;

    char error[256];
};

// Callbacks for media delivery
struct MediaCallbacks {
    void* context;

    // Called when H.264 access unit is ready (from RTP depacketizer)
    // Buffer is valid only during callback; must copy if needed
    void (*on_video_frame)(void* ctx, const uint8_t* annex_b, size_t size, uint32_t rtp_timestamp, bool is_idr);

    // Called when Opus audio frame is ready (decoded PCM)
    // 48kHz stereo, 20ms frames = 1920 samples per channel
    void (*on_audio_frame)(void* ctx, const int16_t* pcm, uint32_t samples_per_channel, uint32_t rtp_timestamp);

    // Called for control messages (text channel)
    void (*on_control_message)(void* ctx, const char* message, size_t len);

    // Called when ICE state changes
    void (*on_ice_state_change)(void* ctx, IceState state);

    // Called when DTLS handshake completes
    void (*on_dtls_connected)(void* ctx);

    // Called when SCTP association is up
    void (*on_sctp_ready)(void* ctx);

    // Called when data channels are open
    void (*on_channels_open)(void* ctx);

    // Called for keyframe requests from decoder
    void (*on_keyframe_needed)(void* ctx);
};

// STUN/TURN server configuration
struct IceServer {
    char urls[256];      // e.g., "stun:stun.l.google.com:19302" or "turn:host:port?transport=udp"
    char username[64];   // For TURN
    char credential[64]; // For TURN
};

// Configuration for WebRTC session
struct Config {
    IceServer ice_servers[4];
    uint32_t ice_server_count;

    // Video settings
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t max_bitrate_kbps;

    // Audio settings
    bool enable_audio;

    // Callbacks
    MediaCallbacks callbacks;

    // Network settings
    uint32_t stun_timeout_ms;
    uint32_t ice_check_interval_ms;
    uint32_t max_ice_checks;
};

// Initialize WebRTC subsystem (call once at startup)
// Returns 0 on success, negative on failure
int init();

// Shutdown WebRTC subsystem (call at exit)
void shutdown();

// Create a new WebRTC session with given config
// Must be called before any other session functions
// Returns 0 on success, negative on failure
int create_session(const Config* config);

// Destroy current session
void destroy_session();

// Get local SDP offer to send to xCloud via /sdp endpoint
// Returns NULL on failure, pointer valid until next session operation
const char* get_local_offer();

// Add remote ICE candidate (from xCloud /candidate endpoint)
// Can be called multiple times before setting remote answer
// Returns 0 on success, negative on failure
int add_ice_candidate(const char* candidate_sdp);

// Set remote answer (from xCloud /answer endpoint)
// MUST be called AFTER all remote candidates are added
// Returns 0 on success, negative on failure
int set_remote_answer(const char* answer_sdp);

// Start the WebRTC pump (must be called regularly from worker thread)
// Returns 0 on success, negative on failure
int start_pump();

// Stop the WebRTC pump
void stop_pump();

// Pump the WebRTC state machine (call periodically, e.g., every 1ms)
// Returns 0 on success, negative on failure
int pump();

// Send gamepad input (call at 60Hz from render thread or worker)
// State is copied internally; call can be made every frame
void send_gamepad(const controller_input::Snapshot& state, double session_time_ms, unsigned int deadzone);

// Request a keyframe (PLI + application message)
void request_keyframe();

// Get current diagnostics
const Diagnostics* get_diagnostics();

// Get current ICE state
IceState get_ice_state();

// Check if session is ready for media
bool is_session_ready();

// Set STUN/TURN servers from xCloud /configuration response
// Called before create_session if servers are known
void set_ice_servers(const IceServer* servers, uint32_t count);

} // namespace webrtc_xbox