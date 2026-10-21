#ifndef XBOXTLS_TLS13_H
#define XBOXTLS_TLS13_H

#include "error.h"
#include "sha256.h"
#include "types.h"

namespace xboxtls {

enum ContentType {
    CONTENT_CHANGE_CIPHER_SPEC = 20,
    CONTENT_ALERT = 21,
    CONTENT_HANDSHAKE = 22,
    CONTENT_APPLICATION_DATA = 23
};

enum HandshakeType {
    HS_CLIENT_HELLO = 1,
    HS_SERVER_HELLO = 2,
    HS_NEW_SESSION_TICKET = 4,
    HS_ENCRYPTED_EXTENSIONS = 8,
    HS_CERTIFICATE = 11,
    HS_CERTIFICATE_VERIFY = 15,
    HS_FINISHED = 20,
    HS_KEY_UPDATE = 24
};

enum AlertLevel { ALERT_WARNING = 1, ALERT_FATAL = 2 };

enum AlertDescription {
    ALERT_CLOSE_NOTIFY = 0,
    ALERT_UNEXPECTED_MESSAGE = 10,
    ALERT_BAD_RECORD_MAC = 20,
    ALERT_RECORD_OVERFLOW = 22,
    ALERT_HANDSHAKE_FAILURE = 40,
    ALERT_BAD_CERTIFICATE = 42,
    ALERT_UNSUPPORTED_CERTIFICATE = 43,
    ALERT_CERTIFICATE_REVOKED = 44,
    ALERT_CERTIFICATE_EXPIRED = 45,
    ALERT_CERTIFICATE_UNKNOWN = 46,
    ALERT_ILLEGAL_PARAMETER = 47,
    ALERT_UNKNOWN_CA = 48,
    ALERT_ACCESS_DENIED = 49,
    ALERT_DECODE_ERROR = 50,
    ALERT_DECRYPT_ERROR = 51,
    ALERT_PROTOCOL_VERSION = 70,
    ALERT_INTERNAL_ERROR = 80,
    ALERT_USER_CANCELED = 90,
    ALERT_MISSING_EXTENSION = 109,
    ALERT_UNSUPPORTED_EXTENSION = 110,
    ALERT_UNRECOGNIZED_NAME = 112,
    ALERT_BAD_CERTIFICATE_STATUS_RESPONSE = 113,
    ALERT_UNKNOWN_PSK_IDENTITY = 115,
    ALERT_CERTIFICATE_REQUIRED = 116,
    ALERT_NO_APPLICATION_PROTOCOL = 120
};

enum CipherSuite {
    TLS_AES_128_GCM_SHA256 = 0x1301,
    TLS_AES_256_GCM_SHA384 = 0x1302,
    TLS_CHACHA20_POLY1305_SHA256 = 0x1303
};

enum NamedGroup { GROUP_SECP256R1 = 0x0017, GROUP_X25519 = 0x001D };

struct TrafficKeysAes128Gcm {
    xt_u8 key[16];
    xt_u8 iv[12];
    xt_u64 sequence_number;
    TrafficKeysAes128Gcm();
};

struct RecordHeader {
    xt_u8 type;
    xt_u16 legacy_version;
    xt_u16 length;
};

Error parse_record_header(ByteSpan input, RecordHeader* out);
void make_record_nonce(const xt_u8 iv[12], xt_u64 sequence_number, xt_u8 nonce[12]);
Error derive_traffic_keys_sha256(ByteSpan traffic_secret, TrafficKeysAes128Gcm* out);
Error update_traffic_secret_sha256(xt_u8 traffic_secret[32], TrafficKeysAes128Gcm* keys);
Error protect_record_aes128_gcm(TrafficKeysAes128Gcm* keys, xt_u8 inner_type, ByteSpan content,
                                size_t zero_padding, MutableByteSpan out, size_t* written);
Error unprotect_record_aes128_gcm(TrafficKeysAes128Gcm* keys, ByteSpan record, xt_u8* inner_type,
                                  MutableByteSpan plaintext, size_t* written);

class TranscriptHash {
  public:
    TranscriptHash();
    void reset();
    void update(ByteSpan bytes);
    void snapshot(xt_u8 out[32]) const;

  private:
    Sha256 sha_;
};

struct KeyScheduleSha256 {
    xt_u8 early_secret[32];
    xt_u8 handshake_secret[32];
    xt_u8 client_handshake_traffic_secret[32];
    xt_u8 server_handshake_traffic_secret[32];
    xt_u8 master_secret[32];
    xt_u8 client_application_traffic_secret[32];
    xt_u8 server_application_traffic_secret[32];

    KeyScheduleSha256();
    Error derive_early_secret(ByteSpan psk);
    Error derive_handshake_secret(ByteSpan ecdhe, ByteSpan transcript_hash);
    Error derive_master_secret(ByteSpan transcript_hash);
};

struct ServerHelloInfo {
    xt_u8 random[32];
    xt_u8 session_id[32];
    size_t session_id_length;
    xt_u16 cipher_suite;
    xt_u16 selected_version;
    NamedGroup group;
    xt_u8 key_share[65];
    size_t key_share_length;
    bool hello_retry_request;
    xt_u8 cookie[512];
    size_t cookie_length;
    ServerHelloInfo();
};

Error parse_server_hello(ByteSpan handshake, ServerHelloInfo* out);

struct HandshakeMessage {
    xt_u8 type;
    ByteSpan body;
    ByteSpan encoded;
    HandshakeMessage();
};

Error parse_handshake_message(ByteSpan input, HandshakeMessage* out, size_t* consumed);

class HandshakeDeframer {
  public:
    explicit HandshakeDeframer(MutableByteSpan storage);
    void reset();
    size_t buffered() const;
    Error append(ByteSpan bytes);
    Error peek(HandshakeMessage* out) const;
    Error consume();

  private:
    MutableByteSpan storage_;
    size_t used_;
};

struct EncryptedExtensionsInfo {
    ByteSpan alpn;
    bool has_alpn;
    EncryptedExtensionsInfo();
};

struct CertificateInfo {
    ByteSpan request_context;
    ByteSpan certificate_list;
    CertificateInfo();
};

struct CertificateVerifyInfo {
    xt_u16 algorithm;
    ByteSpan signature;
    CertificateVerifyInfo();
};

Error parse_encrypted_extensions(const HandshakeMessage& msg, EncryptedExtensionsInfo* out);
Error parse_certificate(const HandshakeMessage& msg, CertificateInfo* out);
Error first_certificate_der(const CertificateInfo& info, ByteSpan* cert_der, ByteSpan* extensions);
Error parse_certificate_verify(const HandshakeMessage& msg, CertificateVerifyInfo* out);
Error finished_key_sha256(ByteSpan traffic_secret, xt_u8 out[32]);
Error compute_finished_verify_data_sha256(ByteSpan traffic_secret, ByteSpan transcript_hash,
                                          xt_u8 out[32]);
Error verify_finished_sha256(ByteSpan traffic_secret, ByteSpan transcript_hash,
                             const HandshakeMessage& msg);
Error build_finished_sha256(ByteSpan traffic_secret, ByteSpan transcript_hash, MutableByteSpan out,
                            size_t* written);

struct ClientHelloParams {
    ByteSpan random32;
    ByteSpan session_id;
    ByteSpan key_share;
    NamedGroup group;
    const char* server_name;
    const char* alpn;
    ByteSpan cookie;
    ClientHelloParams();
};

Error build_client_hello(const ClientHelloParams& p, MutableByteSpan out, size_t* written);

} // namespace xboxtls

#endif
