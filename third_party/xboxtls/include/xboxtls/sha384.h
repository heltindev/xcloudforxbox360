#ifndef XBOXTLS_SHA384_H
#define XBOXTLS_SHA384_H

#include "types.h"

namespace xboxtls {
// FIPS 180-4 SHA-384, 1024-bit blocks / 384-bit output.
// Portable on little/big endian; no host hashing or OpenSSL dependency.
class Sha384 {
  public:
    Sha384();
    void reset();
    void update(const void* data, size_t len);
    void final(xt_u8 out[48]);
  private:
    void transform(const xt_u8 block[128]);
    xt_u64 state_[8];
    xt_u64 total_bytes_;
    xt_u8 buffer_[128];
    size_t buffered_;
};
void sha384(ByteSpan in, xt_u8 out[48]);
} // namespace xboxtls
#endif
