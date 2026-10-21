#include "xboxtls/endian.h"
#include "xboxtls/tls13.h"
#include <string.h>

namespace xboxtls {

Error parse_record_header(ByteSpan input, RecordHeader* out) {
    if (!out || !input.data)
        return XT_ERR_INVALID_ARGUMENT;
    if (input.size < 5)
        return XT_ERR_BUFFER_TOO_SMALL;
    out->type = input.data[0];
    out->legacy_version = load_be16(input.data + 1);
    out->length = load_be16(input.data + 3);
    if (out->length > 16640)
        return XT_ERR_BAD_RECORD;
    return XT_OK;
}

void make_record_nonce(const xt_u8 iv[12], xt_u64 seq, xt_u8 nonce[12]) {
    memcpy(nonce, iv, 12);
    xt_u8 s[8];
    for (int i = 7; i >= 0; --i) {
        s[i] = (xt_u8)seq;
        seq >>= 8;
    }
    for (int i = 0; i < 8; ++i)
        nonce[4 + i] ^= s[i];
}

} // namespace xboxtls
