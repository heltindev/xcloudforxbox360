#include "xboxtls/connection.h"
#include "xboxtls/endian.h"
#include "xboxtls/p256.h"
#include "xboxtls/x25519.h"
#include "xboxtls/native_verifier.h"
#include <string.h>

namespace xboxtls {

ClientConfig::ClientConfig()
    : platform(0),
      stream(0),
      verifier(0),
      hostname(0),
      alpn(0),
      preferred_group(GROUP_X25519),
      application_record_padding(0) {}

TlsClient::TlsClient()
    : state_(CLIENT_IDLE), client_hello_len_(0),
      deframer_(MutableByteSpan(handshake_storage_, sizeof(handshake_storage_))),
      certificate_storage_len_(0), pending_app_offset_(0), pending_app_length_(0),
      last_alert_level_(0), last_alert_description_(0) {
    memset(private_key_, 0, sizeof(private_key_));
    memset(public_key_, 0, sizeof(public_key_));
    memset(client_hello_, 0, sizeof(client_hello_));
    memset(certificate_storage_, 0, sizeof(certificate_storage_));
    memset(negotiated_alpn_, 0, sizeof(negotiated_alpn_));
    memset(pending_app_, 0, sizeof(pending_app_));
}
TlsClient::~TlsClient() {
    wipe_secrets();
}
void TlsClient::wipe_secrets() {
    memset(private_key_, 0, sizeof(private_key_));
    memset(public_key_, 0, sizeof(public_key_));
    memset(&schedule_, 0, sizeof(schedule_));
    memset(&client_hs_keys_, 0, sizeof(client_hs_keys_));
    memset(&server_hs_keys_, 0, sizeof(server_hs_keys_));
    memset(&client_app_keys_, 0, sizeof(client_app_keys_));
    memset(&server_app_keys_, 0, sizeof(server_app_keys_));
}
void TlsClient::fail() {
    state_ = CLIENT_FAILED;
    wipe_secrets();
    if (config_.stream)
        config_.stream->close();
}

Error TlsClient::read_record(xt_u8* record, size_t capacity, size_t* length) {
    if (!record || capacity < 5 || !length || !config_.stream)
        return XT_ERR_INVALID_ARGUMENT;
    size_t have = 0;
    while (have < 5) {
        size_t n = 0;
        Error e = config_.stream->recv_some(MutableByteSpan(record + have, 5 - have), &n);
        if (e != XT_OK)
            return e;
        if (!n)
            return XT_ERR_CLOSED;
        have += n;
    }
    RecordHeader h;
    Error e = parse_record_header(ByteSpan(record, 5), &h);
    if (e != XT_OK)
        return e;
    if (h.length > 16640 || 5 + (size_t)h.length > capacity)
        return XT_ERR_BAD_RECORD;
    size_t total = 5 + (size_t)h.length;
    while (have < total) {
        size_t n = 0;
        e = config_.stream->recv_some(MutableByteSpan(record + have, total - have), &n);
        if (e != XT_OK)
            return e;
        if (!n)
            return XT_ERR_CLOSED;
        have += n;
    }
    *length = total;
    return XT_OK;
}

Error TlsClient::send_plain_record(xt_u8 type, ByteSpan payload) {
    if (!config_.stream || payload.size > 16384)
        return XT_ERR_INVALID_ARGUMENT;
    xt_u8 hdr[5];
    hdr[0] = type;
    hdr[1] = 3;
    hdr[2] = 3;
    store_be16(hdr + 3, (xt_u16)payload.size);
    Error e = config_.stream->send_all(ByteSpan(hdr, 5));
    if (e != XT_OK)
        return e;
    return payload.size ? config_.stream->send_all(payload) : XT_OK;
}
Error TlsClient::send_protected(xt_u8 inner_type, ByteSpan payload) {
    xt_u8 record[16645];
    size_t n = 0;
    Error e = protect_record_aes128_gcm(
        state_ == CLIENT_CONNECTED ? &client_app_keys_ : &client_hs_keys_, inner_type, payload, 0,
        MutableByteSpan(record, sizeof(record)), &n);
    if (e != XT_OK)
        return e;
    return config_.stream->send_all(ByteSpan(record, n));
}

Error TlsClient::connect(const ClientConfig& c) {
    if (state_ != CLIENT_IDLE || !c.platform || !c.stream || !c.verifier || !c.hostname ||
        !*c.hostname)
        return XT_ERR_INVALID_ARGUMENT;
    config_ = c;
    state_ = CLIENT_HANDSHAKING;
    native_auth_set_step(1000); // Begin TLS connection
    transcript_.reset();
    deframer_.reset();
    certificate_storage_len_ = 0;
    negotiated_alpn_[0] = 0;
    pending_app_offset_ = 0;
    pending_app_length_ = 0;
    last_alert_level_ = 0;
    last_alert_description_ = 0;
    xt_u8 random[32], sid[32];
    Error e = c.platform->random_bytes(MutableByteSpan(random, 32));
    if (e != XT_OK) {
        fail();
        return e;
    }
    e = c.platform->random_bytes(MutableByteSpan(sid, 32));
    if (e != XT_OK) {
        fail();
        return e;
    }

    NamedGroup active_group = c.preferred_group;
    size_t public_len = active_group == GROUP_SECP256R1 ? 65 : 32;
    if (active_group == GROUP_SECP256R1) {
        e = XT_ERR_VERIFY;
        for (int attempt = 0; attempt < 16 && e != XT_OK; ++attempt) {
            e = c.platform->random_bytes(MutableByteSpan(private_key_, 32));
            if (e != XT_OK) {
                fail();
                return e;
            }
            e = p256_public_from_private(private_key_, public_key_);
        }
    } else if (active_group == GROUP_X25519) {
        e = c.platform->random_bytes(MutableByteSpan(private_key_, 32));
        if (e == XT_OK) {
            private_key_[0] &= 248;
            private_key_[31] &= 127;
            private_key_[31] |= 64;
            e = x25519_public_from_private(private_key_, public_key_);
        }
    } else {
        e = XT_ERR_UNSUPPORTED;
    }
    if (e != XT_OK) {
        fail();
        return e;
    }
    native_auth_set_step(1010); // Ephemeral public key generated
    ClientHelloParams p;
    p.random32 = ByteSpan(random, 32);
    p.session_id = ByteSpan(sid, 32);
    p.key_share = ByteSpan(public_key_, public_len);
    p.group = active_group;
    p.server_name = c.hostname;
    p.alpn = c.alpn;
    e = build_client_hello(p, MutableByteSpan(client_hello_, sizeof(client_hello_)),
                           &client_hello_len_);
    if (e != XT_OK) {
        memset(random, 0, 32);
        memset(sid, 0, 32);
        fail();
        return e;
    }
    transcript_.update(ByteSpan(client_hello_, client_hello_len_));
    e = send_plain_record(CONTENT_HANDSHAKE, ByteSpan(client_hello_, client_hello_len_));
    if (e != XT_OK) {
        memset(random, 0, 32);
        memset(sid, 0, 32);
        fail();
        return e;
    }

    native_auth_set_step(1020); // ClientHello sent
    xt_u8 record[16645];
    size_t rn = 0;
    ServerHelloInfo sh;
    bool got_sh = false;
    bool had_hrr = false;
    // ServerHello and HelloRetryRequest are handshake messages and may be fragmented across
    // records.
    deframer_.reset();
    while (!got_sh) {
        HandshakeMessage hm;
        e = deframer_.peek(&hm);
        if (e == XT_ERR_BUFFER_TOO_SMALL) {
            e = read_record(record, sizeof(record), &rn);
            if (e != XT_OK) {
                memset(random, 0, 32);
                memset(sid, 0, 32);
                fail();
                return e;
            }
            RecordHeader rh;
            e = parse_record_header(ByteSpan(record, rn), &rh);
            if (e != XT_OK) {
                fail();
                return e;
            }
            if (rh.type == CONTENT_CHANGE_CIPHER_SPEC) {
                if (rh.length != 1 || record[5] != 1) {
                    fail();
                    return XT_ERR_BAD_RECORD;
                }
                continue;
            }
            if (rh.type == CONTENT_ALERT) {
                Error ae = handle_alert(ByteSpan(record + 5, rh.length));
                fail();
                return ae;
            }
            if (rh.type != CONTENT_HANDSHAKE) {
                fail();
                return XT_ERR_BAD_HANDSHAKE;
            }
            e = deframer_.append(ByteSpan(record + 5, rh.length));
            if (e != XT_OK) {
                fail();
                return e;
            }
            continue;
        }
        if (e != XT_OK || hm.type != HS_SERVER_HELLO) {
            fail();
            return XT_ERR_BAD_HANDSHAKE;
        }
        native_auth_set_step(1030); // Received ServerHello record
        e = parse_server_hello(hm.encoded, &sh);
        if (e != XT_OK) {
            fail();
            return e;
        }
        if (sh.session_id_length != 32 || memcmp(sh.session_id, sid, 32) != 0) {
            fail();
            return XT_ERR_BAD_HANDSHAKE;
        }
        if (sh.hello_retry_request) {
            native_auth_set_step(1040); // HelloRetryRequest
            if (had_hrr || sh.group == active_group) {
                fail();
                return XT_ERR_BAD_HANDSHAKE;
            }
            had_hrr = true;
            xt_u8 ch1_hash[32], message_hash[36];
            transcript_.snapshot(ch1_hash);
            message_hash[0] = 0xfe;
            message_hash[1] = 0;
            message_hash[2] = 0;
            message_hash[3] = 32;
            memcpy(message_hash + 4, ch1_hash, 32);
            transcript_.reset();
            transcript_.update(ByteSpan(message_hash, sizeof(message_hash)));
            transcript_.update(hm.encoded);
            e = deframer_.consume();
            if (e != XT_OK || deframer_.buffered() != 0) {
                fail();
                return XT_ERR_BAD_HANDSHAKE;
            }
            active_group = sh.group;
            if (active_group == GROUP_SECP256R1) {
                e = XT_ERR_VERIFY;
                for (int attempt = 0; attempt < 16 && e != XT_OK; ++attempt) {
                    if (c.platform->random_bytes(MutableByteSpan(private_key_, 32)) != XT_OK) {
                        fail();
                        return XT_ERR_CRYPTO;
                    }
                    e = p256_public_from_private(private_key_, public_key_);
                }
                if (e != XT_OK) {
                    fail();
                    return e;
                }
                public_len = 65;
            } else if (active_group == GROUP_X25519) {
                if (c.platform->random_bytes(MutableByteSpan(private_key_, 32)) != XT_OK) {
                    fail();
                    return XT_ERR_CRYPTO;
                }
                private_key_[0] &= 248;
                private_key_[31] &= 127;
                private_key_[31] |= 64;
                e = x25519_public_from_private(private_key_, public_key_);
                if (e != XT_OK) {
                    fail();
                    return e;
                }
                public_len = 32;
            } else {
                fail();
                return XT_ERR_UNSUPPORTED;
            }
            p.group = active_group;
            p.key_share = ByteSpan(public_key_, public_len);
            p.cookie = ByteSpan(sh.cookie, sh.cookie_length);
            e = build_client_hello(p, MutableByteSpan(client_hello_, sizeof(client_hello_)),
                                   &client_hello_len_);
            if (e != XT_OK) {
                fail();
                return e;
            }
            transcript_.update(ByteSpan(client_hello_, client_hello_len_));
            e = send_plain_record(CONTENT_HANDSHAKE, ByteSpan(client_hello_, client_hello_len_));
            if (e != XT_OK) {
                fail();
                return e;
            }
            continue;
        }
        transcript_.update(hm.encoded);
        e = deframer_.consume();
        if (e != XT_OK || deframer_.buffered() != 0) {
            fail();
            return XT_ERR_BAD_HANDSHAKE;
        }
        got_sh = true;
    }
    deframer_.reset();
    memset(random, 0, 32);
    memset(sid, 0, 32);
    if (sh.group != active_group) {
        fail();
        return XT_ERR_BAD_HANDSHAKE;
    }
    native_auth_set_step(1050); // ServerHello accepted
    xt_u8 shared[32], th[32];
    if (sh.group == GROUP_X25519 && sh.key_share_length == 32)
        e = x25519_shared_secret(private_key_, sh.key_share, shared);
    else if (sh.group == GROUP_SECP256R1 && sh.key_share_length == 65)
        e = p256_shared_secret(private_key_, sh.key_share, shared);
    else
        e = XT_ERR_UNSUPPORTED;
    if (e != XT_OK) {
        fail();
        return e;
    }
    native_auth_set_step(1060); // ECDHE shared secret computed
    e = schedule_.derive_early_secret(ByteSpan(0, 0));
    if (e != XT_OK) {
        memset(shared, 0, 32);
        fail();
        return e;
    }
    transcript_.snapshot(th);
    e = schedule_.derive_handshake_secret(ByteSpan(shared, 32), ByteSpan(th, 32));
    memset(shared, 0, 32);
    if (e != XT_OK) {
        fail();
        return e;
    }
    e = derive_traffic_keys_sha256(ByteSpan(schedule_.client_handshake_traffic_secret, 32),
                                   &client_hs_keys_);
    if (e != XT_OK) {
        fail();
        return e;
    }
    e = derive_traffic_keys_sha256(ByteSpan(schedule_.server_handshake_traffic_secret, 32),
                                   &server_hs_keys_);
    if (e != XT_OK) {
        fail();
        return e;
    }
    native_auth_set_step(1070); // Decode authenticated TLS server flight
    e = process_server_handshake();
    if (e != XT_OK) {
        fail();
        return e;
    }
    native_auth_set_step(1090); // Handshake success
    state_ = CLIENT_CONNECTED;
    return XT_OK;
}

Error TlsClient::process_server_handshake() {
    bool got_finished = false, have_cert = false;
    CertificateInfo cert_info;
    xt_u8 record[16645], plain[16640];
    while (!got_finished) {
        size_t rn = 0;
        Error e = read_record(record, sizeof(record), &rn);
        if (e != XT_OK)
            return e;
        RecordHeader rh;
        e = parse_record_header(ByteSpan(record, rn), &rh);
        if (e != XT_OK)
            return e;
        if (rh.type == CONTENT_CHANGE_CIPHER_SPEC)
            continue;
        if (rh.type == CONTENT_ALERT)
            return handle_alert(ByteSpan(record + 5, rh.length));
        if (rh.type != CONTENT_APPLICATION_DATA)
            return XT_ERR_BAD_RECORD;
        xt_u8 inner = 0;
        size_t pn = 0;
        e = unprotect_record_aes128_gcm(&server_hs_keys_, ByteSpan(record, rn), &inner,
                                        MutableByteSpan(plain, sizeof(plain)), &pn);
        if (e != XT_OK)
            return e;
        if (inner == CONTENT_ALERT)
            return handle_alert(ByteSpan(plain, pn));
        if (inner != CONTENT_HANDSHAKE)
            return XT_ERR_BAD_HANDSHAKE;
        e = deframer_.append(ByteSpan(plain, pn));
        if (e != XT_OK)
            return e;
        for (;;) {
            HandshakeMessage msg;
            e = deframer_.peek(&msg);
            if (e == XT_ERR_BUFFER_TOO_SMALL)
                break;
            if (e != XT_OK)
                return e;
            e = handle_handshake_message(msg, &got_finished, &cert_info, &have_cert);
            if (e != XT_OK)
                return e;
            e = deframer_.consume();
            if (e != XT_OK)
                return e;
            if (got_finished)
                break;
        }
    }
    xt_u8 th[32];
    transcript_.snapshot(th);
    Error e = schedule_.derive_master_secret(ByteSpan(th, 32));
    if (e != XT_OK)
        return e;
    e = derive_traffic_keys_sha256(ByteSpan(schedule_.client_application_traffic_secret, 32),
                                   &client_app_keys_);
    if (e != XT_OK)
        return e;
    e = derive_traffic_keys_sha256(ByteSpan(schedule_.server_application_traffic_secret, 32),
                                   &server_app_keys_);
    if (e != XT_OK)
        return e;
    xt_u8 fin[64];
    size_t fn = 0;
    transcript_.snapshot(th);
    e = build_finished_sha256(ByteSpan(schedule_.client_handshake_traffic_secret, 32),
                              ByteSpan(th, 32), MutableByteSpan(fin, sizeof(fin)), &fn);
    if (e != XT_OK)
        return e;
    e = send_protected(CONTENT_HANDSHAKE, ByteSpan(fin, fn));
    if (e != XT_OK)
        return e;
    transcript_.update(ByteSpan(fin, fn));
    return XT_OK;
}

Error TlsClient::handle_handshake_message(const HandshakeMessage& msg, bool* got_finished,
                                          CertificateInfo* cert_info, bool* have_cert) {
    if (!got_finished || !cert_info || !have_cert)
        return XT_ERR_INVALID_ARGUMENT;
    Error e;
    if (msg.type == HS_ENCRYPTED_EXTENSIONS) {
        EncryptedExtensionsInfo ee;
        e = parse_encrypted_extensions(msg, &ee);
        if (e != XT_OK)
            return e;
        if (ee.has_alpn) {
            if (ee.alpn.size >= sizeof(negotiated_alpn_))
                return XT_ERR_BAD_HANDSHAKE;
            memcpy(negotiated_alpn_, ee.alpn.data, ee.alpn.size);
            negotiated_alpn_[ee.alpn.size] = 0;
        }
        transcript_.update(msg.encoded);
        return XT_OK;
    }
    if (msg.type == HS_CERTIFICATE) {
        if (msg.encoded.size > sizeof(certificate_storage_))
            return XT_ERR_BUFFER_TOO_SMALL;
        memcpy(certificate_storage_, msg.encoded.data, msg.encoded.size);
        certificate_storage_len_ = msg.encoded.size;
        HandshakeMessage saved;
        size_t used = 0;
        e = parse_handshake_message(ByteSpan(certificate_storage_, certificate_storage_len_),
                                    &saved, &used);
        if (e != XT_OK)
            return e;
        e = parse_certificate(saved, cert_info);
        if (e != XT_OK)
            return e;
        *have_cert = true;
        transcript_.update(msg.encoded);
        return XT_OK;
    }
    if (msg.type == HS_CERTIFICATE_VERIFY) {
        if (!*have_cert)
            return XT_ERR_BAD_HANDSHAKE;
        CertificateVerifyInfo cv;
        e = parse_certificate_verify(msg, &cv);
        if (e != XT_OK)
            return e;
        xt_u8 th[32];
        transcript_.snapshot(th);
        e = authenticate_server_flight(*cert_info, cv, ByteSpan(th, 32), config_.hostname,
                                       config_.verifier);
        if (e != XT_OK)
            return e;
        transcript_.update(msg.encoded);
        return XT_OK;
    }
    if (msg.type == HS_FINISHED) {
        xt_u8 th[32];
        transcript_.snapshot(th);
        e = verify_finished_sha256(ByteSpan(schedule_.server_handshake_traffic_secret, 32),
                                   ByteSpan(th, 32), msg);
        if (e != XT_OK)
            return e;
        transcript_.update(msg.encoded);
        *got_finished = true;
        return XT_OK;
    }
    return XT_ERR_UNSUPPORTED;
}

Error TlsClient::handle_alert(ByteSpan payload) {
    if (!payload.data || payload.size != 2)
        return XT_ERR_BAD_RECORD;
    last_alert_level_ = payload.data[0];
    last_alert_description_ = payload.data[1];
    if (last_alert_description_ == ALERT_CLOSE_NOTIFY)
        return XT_ERR_CLOSED;
    return XT_ERR_VERIFY;
}

Error TlsClient::process_post_handshake(ByteSpan bytes) {
    Error e = deframer_.append(bytes);
    if (e != XT_OK)
        return e;
    for (;;) {
        HandshakeMessage msg;
        e = deframer_.peek(&msg);
        if (e == XT_ERR_BUFFER_TOO_SMALL)
            return XT_OK;
        if (e != XT_OK)
            return e;
        if (msg.type == HS_NEW_SESSION_TICKET) {
            // Resumption is not implemented yet. RFC 8446 permits clients to ignore tickets.
        } else if (msg.type == HS_KEY_UPDATE) {
            if (msg.body.size != 1 || msg.body.data[0] > 1)
                return XT_ERR_BAD_HANDSHAKE;
            const bool request_update = msg.body.data[0] == 1;
            e = update_traffic_secret_sha256(schedule_.server_application_traffic_secret,
                                             &server_app_keys_);
            if (e != XT_OK)
                return e;
            if (request_update) {
                xt_u8 ku[5] = {HS_KEY_UPDATE, 0, 0, 1, 0};
                e = send_protected(CONTENT_HANDSHAKE, ByteSpan(ku, sizeof(ku)));
                if (e != XT_OK)
                    return e;
                e = update_traffic_secret_sha256(schedule_.client_application_traffic_secret,
                                                 &client_app_keys_);
                if (e != XT_OK)
                    return e;
            }
        } else {
            return XT_ERR_UNSUPPORTED;
        }
        e = deframer_.consume();
        if (e != XT_OK)
            return e;
    }
}


Error TlsClient::key_update(bool request_peer_update) {
    if (state_ != CLIENT_CONNECTED)
        return XT_ERR_CLOSED;

    xt_u8 message[5] = {
        HS_KEY_UPDATE,
        0,
        0,
        1,
        request_peer_update ? 1 : 0
    };

    Error e = send_protected(CONTENT_HANDSHAKE, ByteSpan(message, sizeof(message)));
    if (e != XT_OK) {
        fail();
        return e;
    }

    e = update_traffic_secret_sha256(
        schedule_.client_application_traffic_secret,
        &client_app_keys_);
    if (e != XT_OK) {
        fail();
        return e;
    }

    return XT_OK;
}

Error TlsClient::send(ByteSpan data) {
    if (state_ != CLIENT_CONNECTED)
        return XT_ERR_CLOSED;
    if (data.size && !data.data)
        return XT_ERR_INVALID_ARGUMENT;
    size_t off = 0;
    size_t padding = config_.application_record_padding;
    if (padding > 255)
        padding = 255;

    const size_t max_content = 16623 - padding;
    while (off < data.size) {
        size_t n = data.size - off;
        if (n > max_content)
            n = max_content;

        xt_u8 record[16645];
        size_t written = 0;
        Error e = protect_record_aes128_gcm(
            &client_app_keys_,
            CONTENT_APPLICATION_DATA,
            ByteSpan(data.data + off, n),
            padding,
            MutableByteSpan(record, sizeof(record)),
            &written);
        if (e == XT_OK)
            e = config_.stream->send_all(ByteSpan(record, written));
        if (e != XT_OK) {
            fail();
            return e;
        }
        off += n;
    }
    return XT_OK;
}

Error TlsClient::recv(MutableByteSpan out, size_t* received) {
    if (!received || (!out.data && out.size))
        return XT_ERR_INVALID_ARGUMENT;
    *received = 0;
    if (state_ != CLIENT_CONNECTED)
        return XT_ERR_CLOSED;
    if (pending_app_length_) {
        size_t n = pending_app_length_;
        if (n > out.size)
            n = out.size;
        if (n)
            memcpy(out.data, pending_app_ + pending_app_offset_, n);
        pending_app_offset_ += n;
        pending_app_length_ -= n;
        if (!pending_app_length_)
            pending_app_offset_ = 0;
        *received = n;
        return XT_OK;
    }
    xt_u8 record[16645], plain[16640];
    for (;;) {
        size_t rn = 0;
        Error e = read_record(record, sizeof(record), &rn);
        if (e != XT_OK) {
            fail();
            return e;
        }
        RecordHeader rh;
        e = parse_record_header(ByteSpan(record, rn), &rh);
        if (e != XT_OK) {
            fail();
            return e;
        }
        if (rh.type == CONTENT_CHANGE_CIPHER_SPEC)
            continue;
        if (rh.type != CONTENT_APPLICATION_DATA) {
            fail();
            return XT_ERR_BAD_RECORD;
        }
        xt_u8 inner = 0;
        size_t pn = 0;
        e = unprotect_record_aes128_gcm(&server_app_keys_, ByteSpan(record, rn), &inner,
                                        MutableByteSpan(plain, sizeof(plain)), &pn);
        if (e != XT_OK) {
            fail();
            return e;
        }
        if (inner == CONTENT_APPLICATION_DATA) {
            size_t n = pn;
            if (n > out.size)
                n = out.size;
            if (n)
                memcpy(out.data, plain, n);
            *received = n;
            if (n < pn) {
                size_t remain = pn - n;
                memcpy(pending_app_, plain + n, remain);
                pending_app_offset_ = 0;
                pending_app_length_ = remain;
            }
            return XT_OK;
        }
        if (inner == CONTENT_ALERT) {
            Error ae = handle_alert(ByteSpan(plain, pn));
            if (ae == XT_ERR_CLOSED) {
                state_ = CLIENT_CLOSED;
                config_.stream->close();
                wipe_secrets();
                return XT_ERR_CLOSED;
            }
            fail();
            return ae;
        }
        if (inner == CONTENT_HANDSHAKE) {
            e = process_post_handshake(ByteSpan(plain, pn));
            if (e != XT_OK) {
                fail();
                return e;
            }
            continue;
        }
        fail();
        return XT_ERR_BAD_RECORD;
    }
}


Error TlsClient::reset() {
    if (state_ == CLIENT_HANDSHAKING)
        return XT_ERR_INVALID_ARGUMENT;

    if (config_.stream)
        config_.stream->close();

    wipe_secrets();
    config_ = ClientConfig();
    transcript_.reset();
    deframer_.reset();
    certificate_storage_len_ = 0;
    client_hello_len_ = 0;
    negotiated_alpn_[0] = 0;
    pending_app_offset_ = 0;
    pending_app_length_ = 0;
    last_alert_level_ = 0;
    last_alert_description_ = 0;
    state_ = CLIENT_IDLE;
    return XT_OK;
}

Error TlsClient::close() {
    if (state_ == CLIENT_CLOSED)
        return XT_OK;
    if (state_ == CLIENT_CONNECTED) {
        xt_u8 alert[2] = {1, 0};
        (void)send_protected(CONTENT_ALERT, ByteSpan(alert, 2));
    }
    if (config_.stream)
        config_.stream->close();
    state_ = CLIENT_CLOSED;
    wipe_secrets();
    return XT_OK;
}

} // namespace xboxtls
