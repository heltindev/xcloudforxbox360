#ifndef XBOXTLS_SHA256_H
#define XBOXTLS_SHA256_H

#include "types.h"

namespace xboxtls {

class Sha256 {
  public:
    Sha256();
    void reset();
    void update(const void* data, size_t len);
    void final(xt_u8 out[32]);

  private:
    void transform(const xt_u8 block[64]);
    xt_u32 state_[8];
    xt_u64 total_bytes_;
    xt_u8 buffer_[64];
    size_t buffered_;
};

void sha256(ByteSpan in, xt_u8 out[32]);

} // namespace xboxtls
#endif
