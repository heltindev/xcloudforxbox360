#ifndef XBOXTLS_CONNECTION_H
#define XBOXTLS_CONNECTION_H

#include "authentication.h"
#include "platform.h"
#include "tls13.h"

namespace xboxtls {

enum ClientState {
    CLIENT_IDLE = 0,
    CLIENT_HANDSHAKING,
    CLIENT_CONNECTED,
    CLIENT_CLOSED,
    CLIENT_FAILED
};

struct ClientConfig {
    Platform* platform;
    Stream* stream;
    ServerAuthVerifier* verifier;
    const char* hostname;
    const char* alpn;
    NamedGroup preferred_group;
    size_t application_record_padding;
    ClientConfig();
};

class TlsClient {
  public:
    TlsClient();
    ~TlsClient();

    Error connect(const ClientConfig& config);
    Error send(ByteSpan data);
    Error recv(MutableByteSpan out, size_t* received);
    Error close();
    Error reset();
    Error key_update(bool request_peer_update = false);

    ClientState state() const {
        return state_;
    }
    const char* negotiated_alpn() const {
        return negotiated_alpn_;
    }
    xt_u8 last_alert_level() const {
        return last_alert_level_;
    }
    xt_u8 last_alert_description() const {
        return last_alert_description_;
    }

  private:
    TlsClient(const TlsClient&);
    TlsClient& operator=(const TlsClient&);

    Error read_record(xt_u8* record, size_t capacity, size_t* length);
    Error send_plain_record(xt_u8 type, ByteSpan payload);
    Error send_protected(xt_u8 inner_type, ByteSpan payload);
    Error process_server_handshake();
    Error process_post_handshake(ByteSpan bytes);
    Error handle_alert(ByteSpan payload);
    Error handle_handshake_message(const HandshakeMessage& msg, bool* got_server_finished,
                                   CertificateInfo* cert_info, bool* have_cert);
    void fail();
    void wipe_secrets();

    ClientState state_;
    ClientConfig config_;
    TranscriptHash transcript_;
    KeyScheduleSha256 schedule_;
    TrafficKeysAes128Gcm client_hs_keys_;
    TrafficKeysAes128Gcm server_hs_keys_;
    TrafficKeysAes128Gcm client_app_keys_;
    TrafficKeysAes128Gcm server_app_keys_;
    xt_u8 private_key_[32];
    xt_u8 public_key_[65];
    xt_u8 client_hello_[2048];
    size_t client_hello_len_;
    xt_u8 handshake_storage_[32768];
    HandshakeDeframer deframer_;
    xt_u8 certificate_storage_[24576];
    size_t certificate_storage_len_;
    char negotiated_alpn_[256];
    xt_u8 pending_app_[16384];
    size_t pending_app_offset_;
    size_t pending_app_length_;
    xt_u8 last_alert_level_;
    xt_u8 last_alert_description_;
};

} // namespace xboxtls

#endif
