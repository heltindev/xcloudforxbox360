#ifndef XBOXTLS_P256_H
#define XBOXTLS_P256_H
#include "error.h"
#include "types.h"
namespace xboxtls {
Error p256_public_from_private(const xt_u8 private_key[32], xt_u8 public_key[65]);
Error p256_shared_secret(const xt_u8 private_key[32], const xt_u8 peer_public_key[65],
                         xt_u8 shared_secret[32]);
} // namespace xboxtls
#endif
