#ifndef XBOXTLS_HKDF_H
#define XBOXTLS_HKDF_H

#include "error.h"
#include "types.h"

namespace xboxtls {

void hmac_sha256(ByteSpan key, ByteSpan data, xt_u8 out[32]);
void hkdf_extract_sha256(ByteSpan salt, ByteSpan ikm, xt_u8 out_prk[32]);
Error hkdf_expand_sha256(ByteSpan prk, ByteSpan info, MutableByteSpan out);
Error hkdf_expand_label_sha256(ByteSpan secret, const char* label, ByteSpan context,
                               MutableByteSpan out);

} // namespace xboxtls
#endif
