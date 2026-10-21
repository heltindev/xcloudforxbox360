#include "xboxtls/trust_store.h"
#include "xboxtls/native_verifier.h"

namespace xboxtls {

static size_t read_be24_local(const xt_u8* p) {
    return ((size_t)p[0] << 16) | ((size_t)p[1] << 8) | (size_t)p[2];
}

Error load_compact_trust_store(ByteSpan blob, NativeServerAuthVerifier* verifier,
                               size_t* loaded_count) {
    if (loaded_count)
        *loaded_count = 0;
    if (!verifier || !blob.data || blob.size < 6)
        return XT_ERR_INVALID_ARGUMENT;
    if (blob.data[0] != 'X' || blob.data[1] != 'T' || blob.data[2] != 'S' || blob.data[3] != '1')
        return XT_ERR_BAD_HANDSHAKE;

    const size_t count = ((size_t)blob.data[4] << 8) | (size_t)blob.data[5];
    size_t off = 6;
    Error e = verifier->clear_trust_anchors();
    if (e != XT_OK)
        return e;

    for (size_t i = 0; i < count; ++i) {
        if (off + 3 > blob.size) {
            verifier->clear_trust_anchors();
            return XT_ERR_BAD_HANDSHAKE;
        }
        const size_t len = read_be24_local(blob.data + off);
        off += 3;
        if (!len || len > blob.size - off) {
            verifier->clear_trust_anchors();
            return XT_ERR_BAD_HANDSHAKE;
        }
        e = verifier->add_trust_anchor(ByteSpan(blob.data + off, len));
        if (e != XT_OK) {
            verifier->clear_trust_anchors();
            return e;
        }
        off += len;
    }
    if (off != blob.size) {
        verifier->clear_trust_anchors();
        return XT_ERR_BAD_HANDSHAKE;
    }
    if (loaded_count)
        *loaded_count = count;
    return XT_OK;
}

} // namespace xboxtls
