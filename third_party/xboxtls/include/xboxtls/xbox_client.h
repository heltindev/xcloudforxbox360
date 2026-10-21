#ifndef XBOXTLS_XBOX_CLIENT_H
#define XBOXTLS_XBOX_CLIENT_H

#include "connection.h"
#include "native_verifier.h"
#include "xbox360_platform.h"

#if defined(_XBOX)

namespace xboxtls {

struct XboxTlsOptions {
    const char* trust_store_path;
    xt_u16 port;

    // A single ALPN protocol ("http/1.1") or a comma-separated preference list
    // ("h2,http/1.1"). The server still selects exactly one protocol.
    const char* alpn;
    bool require_alpn;

    NamedGroup preferred_group;
    size_t application_record_padding;

    xt_u32 send_timeout_ms;
    xt_u32 receive_timeout_ms;

    // Optional 32-byte SHA-256 hash of the DER leaf certificate. Pinning is
    // enforced in addition to normal chain and hostname validation.
    const xt_u8* certificate_sha256_pin;

    bool auto_reconnect;
    bool send_close_notify;
    bool request_peer_key_update;

    XboxTlsOptions();
};

struct XboxTlsStats {
    xt_u64 bytes_sent;
    xt_u64 bytes_received;
    xt_u32 connect_attempts;
    xt_u32 successful_connections;
    xt_u32 failed_connections;
    xt_u32 key_updates_sent;
    xt_u32 trust_store_reloads;

    XboxTlsStats();
};

class XboxTlsClient {
  public:
    XboxTlsClient();
    ~XboxTlsClient();

    Error initialize();
    Error initialize(const char* trust_store_path);
    Error initialize(const void* trust_store_data, size_t trust_store_size);
    Error initialize(const XboxTlsOptions& options);

    Error connect(const char* hostname, xt_u16 port = 443, const char* alpn = "http/1.1");
    Error connect(const char* hostname, const XboxTlsOptions& options);

    Error send(ByteSpan data);
    Error send(const void* data, size_t size);

    Error receive(MutableByteSpan out, size_t* received);
    Error receive(void* data, size_t capacity, size_t* received);

    Error key_update(bool request_peer_update = false);
    Error reload_trust_store(const char* trust_store_path);

    Error close();
    void shutdown();

    void set_default_options(const XboxTlsOptions& options);
    const XboxTlsOptions& default_options() const;

    bool initialized() const;
    bool connected() const;
    ClientState state() const;

    Error last_error() const;
    const XboxTlsStats& stats() const;
    void reset_stats();

    const char* negotiated_alpn() const;
    xt_u8 last_alert_level() const;
    xt_u8 last_alert_description() const;

  private:
    XboxTlsClient(const XboxTlsClient&);
    XboxTlsClient& operator=(const XboxTlsClient&);

    Error load_trust_store_file(const char* path);
    Error remember(Error error);
    void free_trust_store();

    Xbox360Platform platform_;
    Xbox360SocketStream socket_;
    NativeServerAuthVerifier verifier_;
    TlsClient tls_;

    XboxTlsOptions defaults_;
    XboxTlsOptions active_options_;
    XboxTlsStats stats_;
    xt_u8* trust_store_data_;
    size_t trust_store_size_;
    Error last_error_;
    bool network_started_;
    bool initialized_;
};

} // namespace xboxtls

#endif
#endif
