#ifndef XBOXTLS_TYPES_H
#define XBOXTLS_TYPES_H

#include <stddef.h>

#if defined(_MSC_VER)
typedef unsigned __int8 xt_u8;
typedef unsigned __int16 xt_u16;
typedef unsigned __int32 xt_u32;
typedef unsigned __int64 xt_u64;
#else
#include <stdint.h>
typedef uint8_t xt_u8;
typedef uint16_t xt_u16;
typedef uint32_t xt_u32;
typedef uint64_t xt_u64;
#endif

namespace xboxtls {

struct ByteSpan {
    const xt_u8* data;
    size_t size;
    ByteSpan() : data(0), size(0) {}
    ByteSpan(const xt_u8* p, size_t n) : data(p), size(n) {}
};

struct MutableByteSpan {
    xt_u8* data;
    size_t size;
    MutableByteSpan() : data(0), size(0) {}
    MutableByteSpan(xt_u8* p, size_t n) : data(p), size(n) {}
};

} // namespace xboxtls

#endif
