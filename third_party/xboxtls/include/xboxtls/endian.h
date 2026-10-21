#ifndef XBOXTLS_ENDIAN_H
#define XBOXTLS_ENDIAN_H

#include "types.h"

namespace xboxtls {

inline xt_u16 load_be16(const xt_u8* p) {
    return (xt_u16)(((xt_u16)p[0] << 8) | p[1]);
}
inline xt_u32 load_be24(const xt_u8* p) {
    return ((xt_u32)p[0] << 16) | ((xt_u32)p[1] << 8) | p[2];
}
inline xt_u32 load_be32(const xt_u8* p) {
    return ((xt_u32)p[0] << 24) | ((xt_u32)p[1] << 16) | ((xt_u32)p[2] << 8) | p[3];
}
inline void store_be16(xt_u8* p, xt_u16 v) {
    p[0] = (xt_u8)(v >> 8);
    p[1] = (xt_u8)v;
}
inline void store_be24(xt_u8* p, xt_u32 v) {
    p[0] = (xt_u8)(v >> 16);
    p[1] = (xt_u8)(v >> 8);
    p[2] = (xt_u8)v;
}
inline void store_be32(xt_u8* p, xt_u32 v) {
    p[0] = (xt_u8)(v >> 24);
    p[1] = (xt_u8)(v >> 16);
    p[2] = (xt_u8)(v >> 8);
    p[3] = (xt_u8)v;
}

} // namespace xboxtls
#endif
