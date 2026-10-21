#ifndef XBOXTLS_PLATFORM_H
#define XBOXTLS_PLATFORM_H

#include "error.h"
#include "types.h"

namespace xboxtls {

class Platform {
  public:
    virtual ~Platform() {}
    virtual Error random_bytes(MutableByteSpan out) = 0;
    virtual xt_u64 unix_time_seconds() = 0;
};

class Stream {
  public:
    virtual ~Stream() {}
    virtual Error send_all(ByteSpan data) = 0;
    virtual Error recv_some(MutableByteSpan buffer, size_t* received) = 0;
    virtual void close() = 0;
};

} // namespace xboxtls

#endif
