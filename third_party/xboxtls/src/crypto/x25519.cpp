#include "xboxtls/x25519.h"
#include <string.h>

namespace xboxtls {
typedef long long i64;
typedef i64 gf[16];
static const gf gf0 = {0};
static const gf gf1 = {1};
static const gf c121665 = {0xDB41, 1};
static void set25519(gf r, const gf a) {
    for (int i = 0; i < 16; ++i)
        r[i] = a[i];
}
static void car25519(gf o) {
    for (int i = 0; i < 16; ++i) {
        o[i] += (i64)1 << 16;
        i64 c = o[i] >> 16;
        o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
        o[i] -= c << 16;
    }
}
static void sel25519(gf p, gf q, int b) {
    i64 c = ~(i64)(b - 1);
    for (int i = 0; i < 16; ++i) {
        i64 t = c & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}
static void pack25519(xt_u8* o, const gf n) {
    gf m, t;
    for (int i = 0; i < 16; ++i)
        t[i] = n[i];
    car25519(t);
    car25519(t);
    car25519(t);
    for (int j = 0; j < 2; ++j) {
        m[0] = t[0] - 0xffed;
        for (int i = 1; i < 15; ++i) {
            m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
            m[i - 1] &= 0xffff;
        }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        int b = (int)((m[15] >> 16) & 1);
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);
    }
    for (int i = 0; i < 16; ++i) {
        o[2 * i] = (xt_u8)t[i];
        o[2 * i + 1] = (xt_u8)(t[i] >> 8);
    }
    memset(m, 0, sizeof(m));
    memset(t, 0, sizeof(t));
}
static void unpack25519(gf o, const xt_u8* n) {
    for (int i = 0; i < 16; ++i)
        o[i] = n[2 * i] + ((i64)n[2 * i + 1] << 8);
    o[15] &= 0x7fff;
}
static void A(gf o, const gf a, const gf b) {
    for (int i = 0; i < 16; ++i)
        o[i] = a[i] + b[i];
}
static void Z(gf o, const gf a, const gf b) {
    for (int i = 0; i < 16; ++i)
        o[i] = a[i] - b[i];
}
static void M(gf o, const gf a, const gf b) {
    i64 t[31];
    for (int i = 0; i < 31; ++i)
        t[i] = 0;
    for (int i = 0; i < 16; ++i)
        for (int j = 0; j < 16; ++j)
            t[i + j] += a[i] * b[j];
    for (int i = 0; i < 15; ++i)
        t[i] += 38 * t[i + 16];
    for (int i = 0; i < 16; ++i)
        o[i] = t[i];
    car25519(o);
    car25519(o);
    memset(t, 0, sizeof(t));
}
static void S(gf o, const gf a) {
    M(o, a, a);
}
static void inv25519(gf o, const gf i) {
    gf c;
    set25519(c, i);
    for (int a = 253; a >= 0; --a) {
        S(c, c);
        if (a != 2 && a != 4)
            M(c, c, i);
    }
    set25519(o, c);
    memset(c, 0, sizeof(c));
}
static void scalarmult(xt_u8 q[32], const xt_u8 n[32], const xt_u8 p[32]) {
    xt_u8 z[32];
    gf x, a, b, c, d, e, f;
    memcpy(z, n, 32);
    z[31] = (xt_u8)((z[31] & 127) | 64);
    z[0] &= 248;
    unpack25519(x, p);
    set25519(a, gf1);
    set25519(b, x);
    set25519(c, gf0);
    set25519(d, gf1);
    for (int i = 254; i >= 0; --i) {
        int r = (z[i >> 3] >> (i & 7)) & 1;
        sel25519(a, b, r);
        sel25519(c, d, r);
        A(e, a, c);
        Z(a, a, c);
        A(c, b, d);
        Z(b, b, d);
        S(d, e);
        S(f, a);
        M(a, c, a);
        M(c, b, e);
        A(e, a, c);
        Z(a, a, c);
        S(b, a);
        Z(c, d, f);
        M(a, c, c121665);
        A(a, a, d);
        M(c, c, a);
        M(a, d, f);
        M(d, b, x);
        S(b, e);
        sel25519(a, b, r);
        sel25519(c, d, r);
    }
    inv25519(c, c);
    M(a, a, c);
    pack25519(q, a);
    memset(z, 0, sizeof(z));
    memset(x, 0, sizeof(x));
    memset(a, 0, sizeof(a));
    memset(b, 0, sizeof(b));
    memset(c, 0, sizeof(c));
    memset(d, 0, sizeof(d));
    memset(e, 0, sizeof(e));
    memset(f, 0, sizeof(f));
}
static bool all_zero(const xt_u8 p[32]) {
    xt_u8 v = 0;
    for (int i = 0; i < 32; ++i)
        v |= p[i];
    return v == 0;
}
Error x25519_public_from_private(const xt_u8 private_key[32], xt_u8 public_key[32]) {
    if (!private_key || !public_key)
        return XT_ERR_INVALID_ARGUMENT;
    xt_u8 base[32] = {9};
    scalarmult(public_key, private_key, base);
    return XT_OK;
}
Error x25519_shared_secret(const xt_u8 private_key[32], const xt_u8 peer_public_key[32],
                           xt_u8 shared_secret[32]) {
    if (!private_key || !peer_public_key || !shared_secret)
        return XT_ERR_INVALID_ARGUMENT;
    scalarmult(shared_secret, private_key, peer_public_key);
    if (all_zero(shared_secret)) {
        memset(shared_secret, 0, 32);
        return XT_ERR_VERIFY;
    }
    return XT_OK;
}
} // namespace xboxtls
