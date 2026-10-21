#ifndef XBOXTLS_AES_GCM_H
#define XBOXTLS_AES_GCM_H

#include "error.h"
#include "types.h"

namespace xboxtls {

Error aes128_gcm_encrypt(const xt_u8 key[16], const xt_u8 nonce[12], ByteSpan aad,
                         ByteSpan plaintext, MutableByteSpan ciphertext, xt_u8 tag[16]);
Error aes128_gcm_decrypt(const xt_u8 key[16], const xt_u8 nonce[12], ByteSpan aad,
                         ByteSpan ciphertext, const xt_u8 tag[16], MutableByteSpan plaintext);

} // namespace xboxtls
#endif
