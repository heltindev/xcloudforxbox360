#include "xboxtls/endian.h"
#include "xboxtls/hkdf.h"
#include "xboxtls/tls13.h"
#include <string.h>

namespace xboxtls {

HandshakeMessage::HandshakeMessage() : type(0), body(), encoded() {}
EncryptedExtensionsInfo::EncryptedExtensionsInfo() : alpn(), has_alpn(false) {}
CertificateInfo::CertificateInfo() : request_context(), certificate_list() {}
CertificateVerifyInfo::CertificateVerifyInfo() : algorithm(0), signature() {}

static xt_u32 load_u24(const xt_u8* p) {
    return ((xt_u32)p[0] << 16) | ((xt_u32)p[1] << 8) | (xt_u32)p[2];
}

Error parse_handshake_message(ByteSpan input, HandshakeMessage* out, size_t* consumed) {
    if (!out || !consumed || (!input.data && input.size))
        return XT_ERR_INVALID_ARGUMENT;
    *consumed = 0;
    if (input.size < 4)
        return XT_ERR_BUFFER_TOO_SMALL;
    const size_t body_len = (size_t)load_u24(input.data + 1);
    if (body_len > 0xFFFFFFu)
        return XT_ERR_BAD_HANDSHAKE;
    const size_t total = body_len + 4;
    if (input.size < total)
        return XT_ERR_BUFFER_TOO_SMALL;
    out->type = input.data[0];
    out->body = ByteSpan(input.data + 4, body_len);
    out->encoded = ByteSpan(input.data, total);
    *consumed = total;
    return XT_OK;
}

HandshakeDeframer::HandshakeDeframer(MutableByteSpan storage) : storage_(storage), used_(0) {}

void HandshakeDeframer::reset() {
    used_ = 0;
}
size_t HandshakeDeframer::buffered() const {
    return used_;
}

Error HandshakeDeframer::append(ByteSpan bytes) {
    if ((!bytes.data && bytes.size) || (!storage_.data && storage_.size))
        return XT_ERR_INVALID_ARGUMENT;
    if (bytes.size > storage_.size - used_)
        return XT_ERR_BUFFER_TOO_SMALL;
    if (bytes.size)
        memcpy(storage_.data + used_, bytes.data, bytes.size);
    used_ += bytes.size;
    return XT_OK;
}

Error HandshakeDeframer::peek(HandshakeMessage* out) const {
    size_t consumed = 0;
    return parse_handshake_message(ByteSpan(storage_.data, used_), out, &consumed);
}

Error HandshakeDeframer::consume() {
    HandshakeMessage m;
    size_t n = 0;
    Error e = parse_handshake_message(ByteSpan(storage_.data, used_), &m, &n);
    if (e != XT_OK)
        return e;
    if (n < used_)
        memmove(storage_.data, storage_.data + n, used_ - n);
    used_ -= n;
    return XT_OK;
}

Error parse_encrypted_extensions(const HandshakeMessage& msg, EncryptedExtensionsInfo* out) {
    if (!out || msg.type != HS_ENCRYPTED_EXTENSIONS || !msg.body.data || msg.body.size < 2)
        return XT_ERR_BAD_HANDSHAKE;
    *out = EncryptedExtensionsInfo();
    const xt_u8* p = msg.body.data;
    size_t remain = msg.body.size;
    const size_t extensions_len = ((size_t)p[0] << 8) | p[1];
    p += 2;
    remain -= 2;
    if (extensions_len != remain)
        return XT_ERR_BAD_HANDSHAKE;
    while (remain) {
        if (remain < 4)
            return XT_ERR_BAD_HANDSHAKE;
        const xt_u16 type = (xt_u16)(((xt_u16)p[0] << 8) | p[1]);
        const size_t len = ((size_t)p[2] << 8) | p[3];
        p += 4;
        remain -= 4;
        if (len > remain)
            return XT_ERR_BAD_HANDSHAKE;
        if (type == 0x0010) { // ALPN
            if (len < 3)
                return XT_ERR_BAD_HANDSHAKE;
            size_t list_len = ((size_t)p[0] << 8) | p[1];
            if (list_len + 2 != len || list_len < 1)
                return XT_ERR_BAD_HANDSHAKE;
            const size_t proto_len = p[2];
            if (proto_len == 0 || proto_len + 1 != list_len)
                return XT_ERR_BAD_HANDSHAKE;
            out->alpn = ByteSpan(p + 3, proto_len);
            out->has_alpn = true;
        }
        p += len;
        remain -= len;
    }
    return XT_OK;
}

Error parse_certificate(const HandshakeMessage& msg, CertificateInfo* out) {
    if (!out || msg.type != HS_CERTIFICATE || !msg.body.data || msg.body.size < 4)
        return XT_ERR_BAD_HANDSHAKE;
    const xt_u8* p = msg.body.data;
    size_t remain = msg.body.size;
    const size_t context_len = p[0];
    ++p;
    --remain;
    if (context_len > remain)
        return XT_ERR_BAD_HANDSHAKE;
    out->request_context = ByteSpan(p, context_len);
    p += context_len;
    remain -= context_len;
    if (remain < 3)
        return XT_ERR_BAD_HANDSHAKE;
    const size_t list_len = load_u24(p);
    p += 3;
    remain -= 3;
    if (list_len != remain)
        return XT_ERR_BAD_HANDSHAKE;
    out->certificate_list = ByteSpan(p, list_len);
    return XT_OK;
}

Error first_certificate_der(const CertificateInfo& info, ByteSpan* cert_der, ByteSpan* extensions) {
    if (!cert_der || !extensions || !info.certificate_list.data || info.certificate_list.size < 5)
        return XT_ERR_BAD_HANDSHAKE;
    const xt_u8* p = info.certificate_list.data;
    size_t remain = info.certificate_list.size;
    const size_t cert_len = load_u24(p);
    p += 3;
    remain -= 3;
    if (cert_len == 0 || cert_len > remain)
        return XT_ERR_BAD_HANDSHAKE;
    *cert_der = ByteSpan(p, cert_len);
    p += cert_len;
    remain -= cert_len;
    if (remain < 2)
        return XT_ERR_BAD_HANDSHAKE;
    const size_t ext_len = ((size_t)p[0] << 8) | p[1];
    p += 2;
    remain -= 2;
    if (ext_len > remain)
        return XT_ERR_BAD_HANDSHAKE;
    *extensions = ByteSpan(p, ext_len);
    return XT_OK;
}

Error parse_certificate_verify(const HandshakeMessage& msg, CertificateVerifyInfo* out) {
    if (!out || msg.type != HS_CERTIFICATE_VERIFY || !msg.body.data || msg.body.size < 4)
        return XT_ERR_BAD_HANDSHAKE;
    const xt_u8* p = msg.body.data;
    out->algorithm = (xt_u16)(((xt_u16)p[0] << 8) | p[1]);
    const size_t sig_len = ((size_t)p[2] << 8) | p[3];
    if (sig_len == 0 || sig_len + 4 != msg.body.size)
        return XT_ERR_BAD_HANDSHAKE;
    out->signature = ByteSpan(p + 4, sig_len);
    return XT_OK;
}

Error finished_key_sha256(ByteSpan traffic_secret, xt_u8 out[32]) {
    if (!out || !traffic_secret.data || traffic_secret.size != 32)
        return XT_ERR_INVALID_ARGUMENT;
    return hkdf_expand_label_sha256(traffic_secret, "finished", ByteSpan(0, 0),
                                    MutableByteSpan(out, 32));
}

Error compute_finished_verify_data_sha256(ByteSpan traffic_secret, ByteSpan transcript_hash,
                                          xt_u8 out[32]) {
    if (!out || !transcript_hash.data || transcript_hash.size != 32)
        return XT_ERR_INVALID_ARGUMENT;
    xt_u8 finished_key[32];
    Error e = finished_key_sha256(traffic_secret, finished_key);
    if (e != XT_OK)
        return e;
    hmac_sha256(ByteSpan(finished_key, 32), transcript_hash, out);
    memset(finished_key, 0, sizeof(finished_key));
    return XT_OK;
}

static bool constant_time_equal(const xt_u8* a, const xt_u8* b, size_t n) {
    xt_u8 diff = 0;
    for (size_t i = 0; i < n; ++i)
        diff |= (xt_u8)(a[i] ^ b[i]);
    return diff == 0;
}

Error verify_finished_sha256(ByteSpan traffic_secret, ByteSpan transcript_hash,
                             const HandshakeMessage& msg) {
    if (msg.type != HS_FINISHED || !msg.body.data || msg.body.size != 32)
        return XT_ERR_BAD_HANDSHAKE;
    xt_u8 expected[32];
    Error e = compute_finished_verify_data_sha256(traffic_secret, transcript_hash, expected);
    if (e != XT_OK)
        return e;
    bool ok = constant_time_equal(expected, msg.body.data, 32);
    memset(expected, 0, sizeof(expected));
    return ok ? XT_OK : XT_ERR_VERIFY;
}

Error build_finished_sha256(ByteSpan traffic_secret, ByteSpan transcript_hash, MutableByteSpan out,
                            size_t* written) {
    if (!written || !out.data)
        return XT_ERR_INVALID_ARGUMENT;
    if (out.size < 36)
        return XT_ERR_BUFFER_TOO_SMALL;
    out.data[0] = HS_FINISHED;
    out.data[1] = 0;
    out.data[2] = 0;
    out.data[3] = 32;
    Error e = compute_finished_verify_data_sha256(traffic_secret, transcript_hash, out.data + 4);
    if (e != XT_OK)
        return e;
    *written = 36;
    return XT_OK;
}

} // namespace xboxtls
