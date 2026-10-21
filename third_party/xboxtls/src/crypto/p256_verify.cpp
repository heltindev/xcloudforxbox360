#include "xboxtls/native_verifier.h"
#include "xboxtls/p256.h"
#include "xboxtls/sha256.h"
#include <string.h>

namespace xboxtls {

struct N256 {
    xt_u32 w[9];
    size_t n;
    N256() : n(0) {
        memset(w, 0, sizeof(w));
    }
};
static void trim(N256& a) {
    while (a.n && a.w[a.n - 1] == 0)
        --a.n;
}
static int cmp(const N256& a, const N256& b) {
    if (a.n != b.n)
        return a.n < b.n ? -1 : 1;
    for (size_t i = a.n; i > 0; --i) {
        if (a.w[i - 1] != b.w[i - 1])
            return a.w[i - 1] < b.w[i - 1] ? -1 : 1;
    }
    return 0;
}
static N256 be(const xt_u8* p, size_t n) {
    N256 a;
    size_t nw = (n + 3) / 4;
    if (nw > 8)
        return a;
    a.n = nw;
    for (size_t i = 0; i < n; ++i) {
        size_t r = n - 1 - i;
        a.w[i / 4] |= ((xt_u32)p[r]) << ((i % 4) * 8);
    }
    trim(a);
    return a;
}
static void subraw(N256& a, const N256& b) {
    xt_u64 br = 0;
    for (size_t i = 0; i < a.n; ++i) {
        xt_u64 av = a.w[i], bv = (i < b.n ? b.w[i] : 0) + br;
        if (av >= bv) {
            a.w[i] = (xt_u32)(av - bv);
            br = 0;
        } else {
            a.w[i] = (xt_u32)(((xt_u64)1 << 32) + av - bv);
            br = 1;
        }
    }
    trim(a);
}
static N256 addm(const N256& a, const N256& b, const N256& m) {
    N256 t;
    t.n = m.n + 1;
    xt_u64 c = 0;
    for (size_t i = 0; i < m.n; ++i) {
        xt_u64 s = c + (i < a.n ? a.w[i] : 0) + (i < b.n ? b.w[i] : 0);
        t.w[i] = (xt_u32)s;
        c = s >> 32;
    }
    t.w[m.n] = (xt_u32)c;
    trim(t);
    if (t.n > m.n || cmp(t, m) >= 0)
        subraw(t, m);
    return t;
}
static N256 subm(const N256& a, const N256& b, const N256& m) {
    if (cmp(a, b) >= 0) {
        N256 r = a;
        subraw(r, b);
        return r;
    }
    N256 d = b;
    subraw(d, a);
    N256 r = m;
    subraw(r, d);
    return r;
}
static size_t nbits(const N256& a) {
    if (!a.n)
        return 0;
    xt_u32 v = a.w[a.n - 1];
    size_t b = 32;
    while (b && ((v >> (b - 1)) & 1) == 0)
        --b;
    return (a.n - 1) * 32 + b;
}
static bool bit(const N256& a, size_t i) {
    size_t w = i / 32;
    return w < a.n && ((a.w[w] >> (i % 32)) & 1) != 0;
}
static N256 mulm(N256 a, const N256& b, const N256& m) {
    N256 r;
    size_t nb = nbits(b);
    for (size_t i = 0; i < nb; ++i) {
        if (bit(b, i))
            r = addm(r, a, m);
        a = addm(a, a, m);
    }
    return r;
}
static N256 powm(const N256& a, const N256& e, const N256& m) {
    N256 r;
    r.n = 1;
    r.w[0] = 1;
    N256 x = a;
    size_t nb = nbits(e);
    for (size_t i = 0; i < nb; ++i) {
        if (bit(e, i))
            r = mulm(r, x, m);
        x = mulm(x, x, m);
    }
    return r;
}
static bool zero(const N256& a) {
    return a.n == 0;
}
static bool eq(const N256& a, const N256& b) {
    return cmp(a, b) == 0;
}
static N256 small(unsigned v) {
    N256 a;
    if (v) {
        a.n = 1;
        a.w[0] = v;
    }
    return a;
}

static const xt_u8 P_BE[32] = {0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const xt_u8 N_BE[32] = {0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
                               0xff, 0xff, 0xff, 0xff, 0xff, 0xbc, 0xe6, 0xfa, 0xad, 0xa7, 0x17,
                               0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc, 0x63, 0x25, 0x51};
static const xt_u8 B_BE[32] = {0x5a, 0xc6, 0x35, 0xd8, 0xaa, 0x3a, 0x93, 0xe7, 0xb3, 0xeb, 0xbd,
                               0x55, 0x76, 0x98, 0x86, 0xbc, 0x65, 0x1d, 0x06, 0xb0, 0xcc, 0x53,
                               0xb0, 0xf6, 0x3b, 0xce, 0x3c, 0x3e, 0x27, 0xd2, 0x60, 0x4b};
static const xt_u8 GX_BE[32] = {0x6b, 0x17, 0xd1, 0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6,
                                0xe5, 0x63, 0xa4, 0x40, 0xf2, 0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb,
                                0x33, 0xa0, 0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96};
static const xt_u8 GY_BE[32] = {0x4f, 0xe3, 0x42, 0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb,
                                0x4a, 0x7c, 0x0f, 0x9e, 0x16, 0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31,
                                0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68, 0x37, 0xbf, 0x51, 0xf5};

struct JP {
    N256 x, y, z;
    bool inf;
    JP() : inf(true) {}
};
static JP dbl(const JP& a, const N256& p) {
    if (a.inf || zero(a.y))
        return JP();
    N256 xx = mulm(a.x, a.x, p), yy = mulm(a.y, a.y, p), yyyy = mulm(yy, yy, p),
         zz = mulm(a.z, a.z, p);
    N256 xpy = addm(a.x, yy, p);
    N256 s = subm(subm(mulm(xpy, xpy, p), xx, p), yyyy, p);
    s = addm(s, s, p);
    N256 zz2 = mulm(zz, zz, p);
    N256 m = subm(xx, zz2, p);
    m = addm(addm(m, m, p), m, p);
    N256 t = subm(mulm(m, m, p), addm(s, s, p), p);
    N256 y3 = subm(mulm(m, subm(s, t, p), p),
                   addm(addm(addm(yyyy, yyyy, p), addm(yyyy, yyyy, p), p),
                        addm(addm(yyyy, yyyy, p), addm(yyyy, yyyy, p), p), p),
                   p);
    N256 z3 = subm(subm(mulm(addm(a.y, a.z, p), addm(a.y, a.z, p), p), yy, p), zz, p);
    JP r;
    r.inf = false;
    r.x = t;
    r.y = y3;
    r.z = z3;
    return r;
}
static JP add_mixed(const JP& a, const N256& qx, const N256& qy, const N256& p) {
    if (a.inf) {
        JP r;
        r.inf = false;
        r.x = qx;
        r.y = qy;
        r.z = small(1);
        return r;
    }
    N256 z1z1 = mulm(a.z, a.z, p), u2 = mulm(qx, z1z1, p), s2 = mulm(qy, mulm(a.z, z1z1, p), p);
    if (eq(u2, a.x)) {
        if (eq(s2, a.y))
            return dbl(a, p);
        return JP();
    }
    N256 h = subm(u2, a.x, p), hh = mulm(h, h, p), i = addm(addm(hh, hh, p), addm(hh, hh, p), p),
         j = mulm(h, i, p), rr = addm(subm(s2, a.y, p), subm(s2, a.y, p), p), v = mulm(a.x, i, p);
    N256 x3 = subm(subm(mulm(rr, rr, p), j, p), addm(v, v, p), p);
    N256 y3 = subm(mulm(rr, subm(v, x3, p), p), addm(mulm(a.y, j, p), mulm(a.y, j, p), p), p);
    N256 z3 = subm(subm(mulm(addm(a.z, h, p), addm(a.z, h, p), p), z1z1, p), hh, p);
    JP r;
    r.inf = false;
    r.x = x3;
    r.y = y3;
    r.z = z3;
    return r;
}
static JP addj(const JP& a, const JP& b, const N256& p) {
    if (a.inf)
        return b;
    if (b.inf)
        return a;
    N256 z1z1 = mulm(a.z, a.z, p), z2z2 = mulm(b.z, b.z, p), u1 = mulm(a.x, z2z2, p),
         u2 = mulm(b.x, z1z1, p), s1 = mulm(a.y, mulm(b.z, z2z2, p), p),
         s2 = mulm(b.y, mulm(a.z, z1z1, p), p);
    if (eq(u1, u2)) {
        if (eq(s1, s2))
            return dbl(a, p);
        return JP();
    }
    N256 h = subm(u2, u1, p), twoh = addm(h, h, p), i = mulm(twoh, twoh, p), j = mulm(h, i, p),
         rr = addm(subm(s2, s1, p), subm(s2, s1, p), p), v = mulm(u1, i, p);
    N256 x3 = subm(subm(mulm(rr, rr, p), j, p), addm(v, v, p), p);
    N256 y3 = subm(mulm(rr, subm(v, x3, p), p), addm(mulm(s1, j, p), mulm(s1, j, p), p), p);
    N256 zsum = addm(a.z, b.z, p);
    N256 z3 = mulm(subm(subm(mulm(zsum, zsum, p), z1z1, p), z2z2, p), h, p);
    JP r;
    r.inf = false;
    r.x = x3;
    r.y = y3;
    r.z = z3;
    return r;
}
static JP scalar(const N256& k, const N256& x, const N256& y, const N256& p) {
    JP r;
    size_t nb = nbits(k);
    for (size_t i = nb; i > 0; --i) {
        r = dbl(r, p);
        if (bit(k, i - 1))
            r = add_mixed(r, x, y, p);
    }
    return r;
}
static bool affine_x(const JP& a, const N256& p, N256* out) {
    if (a.inf || !out)
        return false;
    N256 pm2 = p;
    N256 two = small(2);
    subraw(pm2, two);
    N256 zi = powm(a.z, pm2, p), zi2 = mulm(zi, zi, p);
    *out = mulm(a.x, zi2, p);
    return true;
}
static bool affine_xy(const JP& a, const N256& p, N256* outx, N256* outy) {
    if (a.inf || !outx || !outy)
        return false;
    N256 pm2 = p;
    subraw(pm2, small(2));
    N256 zi = powm(a.z, pm2, p), zi2 = mulm(zi, zi, p), zi3 = mulm(zi2, zi, p);
    *outx = mulm(a.x, zi2, p);
    *outy = mulm(a.y, zi3, p);
    return true;
}
static void to_be32(const N256& a, xt_u8 out[32]) {
    memset(out, 0, 32);
    for (size_t i = 0; i < a.n && i < 8; ++i) {
        xt_u32 v = a.w[i];
        size_t o = 32 - 4 * (i + 1);
        out[o] = (xt_u8)(v >> 24);
        out[o + 1] = (xt_u8)(v >> 16);
        out[o + 2] = (xt_u8)(v >> 8);
        out[o + 3] = (xt_u8)v;
    }
}
static bool valid_private(const N256& d, const N256& n) {
    return !zero(d) && cmp(d, n) < 0;
}

Error p256_public_from_private(const xt_u8 private_key[32], xt_u8 public_key[65]) {
    if (!private_key || !public_key)
        return XT_ERR_INVALID_ARGUMENT;
    N256 p = be(P_BE, 32), n = be(N_BE, 32), gx = be(GX_BE, 32), gy = be(GY_BE, 32),
         d = be(private_key, 32);
    if (!valid_private(d, n))
        return XT_ERR_VERIFY;
    JP q = scalar(d, gx, gy, p);
    N256 x, y;
    if (!affine_xy(q, p, &x, &y))
        return XT_ERR_VERIFY;
    public_key[0] = 0x04;
    to_be32(x, public_key + 1);
    to_be32(y, public_key + 33);
    return XT_OK;
}

Error p256_shared_secret(const xt_u8 private_key[32], const xt_u8 peer_public_key[65],
                         xt_u8 shared_secret[32]) {
    if (!private_key || !peer_public_key || !shared_secret)
        return XT_ERR_INVALID_ARGUMENT;
    if (peer_public_key[0] != 0x04)
        return XT_ERR_VERIFY;
    N256 p = be(P_BE, 32), n = be(N_BE, 32), bb = be(B_BE, 32), d = be(private_key, 32);
    if (!valid_private(d, n))
        return XT_ERR_VERIFY;
    N256 qx = be(peer_public_key + 1, 32), qy = be(peer_public_key + 33, 32);
    if (cmp(qx, p) >= 0 || cmp(qy, p) >= 0)
        return XT_ERR_VERIFY;
    N256 lhs = mulm(qy, qy, p), x2 = mulm(qx, qx, p), x3 = mulm(x2, qx, p),
         threeX = addm(addm(qx, qx, p), qx, p), rhs = addm(subm(x3, threeX, p), bb, p);
    if (!eq(lhs, rhs))
        return XT_ERR_VERIFY;
    JP q = scalar(d, qx, qy, p);
    N256 x, y;
    if (!affine_xy(q, p, &x, &y))
        return XT_ERR_VERIFY;
    to_be32(x, shared_secret);
    return XT_OK;
}

struct Dtlv {
    xt_u8 tag;
    ByteSpan v;
    size_t next;
};
static bool dtlv(ByteSpan in, size_t off, Dtlv* o) {
    if (!o || !in.data || off >= in.size)
        return false;
    size_t p = off;
    o->tag = in.data[p++];
    if (p >= in.size)
        return false;
    xt_u8 lb = in.data[p++];
    size_t len = 0;
    if (!(lb & 0x80))
        len = lb;
    else {
        size_t n = lb & 0x7f;
        if (!n || n > 2 || p + n > in.size)
            return false;
        for (size_t i = 0; i < n; ++i)
            len = (len << 8) | in.data[p++];
    }
    if (len > in.size - p)
        return false;
    o->v = ByteSpan(in.data + p, len);
    o->next = p + len;
    return true;
}
static bool parse_sig(ByteSpan der, N256* r, N256* s) {
    Dtlv seq;
    if (!dtlv(der, 0, &seq) || seq.tag != 0x30 || seq.next != der.size)
        return false;
    Dtlv a, b;
    if (!dtlv(seq.v, 0, &a) || a.tag != 0x02 || !dtlv(seq.v, a.next, &b) || b.tag != 0x02 ||
        b.next != seq.v.size)
        return false;
    ByteSpan av = a.v, bv = b.v;
    if (av.size > 1 && av.data[0] == 0)
        av = ByteSpan(av.data + 1, av.size - 1);
    if (bv.size > 1 && bv.data[0] == 0)
        bv = ByteSpan(bv.data + 1, bv.size - 1);
    if (!av.size || av.size > 32 || !bv.size || bv.size > 32)
        return false;
    *r = be(av.data, av.size);
    *s = be(bv.data, bv.size);
    return true;
}

Error native_p256_ecdsa_sha256_verify(ByteSpan public_key, ByteSpan message, ByteSpan signature) {
    if (public_key.size != 65 || public_key.data[0] != 0x04)
        return XT_ERR_VERIFY;
    N256 p = be(P_BE, 32), n = be(N_BE, 32), bb = be(B_BE, 32), gx = be(GX_BE, 32),
         gy = be(GY_BE, 32);
    N256 qx = be(public_key.data + 1, 32), qy = be(public_key.data + 33, 32);
    if (cmp(qx, p) >= 0 || cmp(qy, p) >= 0)
        return XT_ERR_VERIFY;
    N256 lhs = mulm(qy, qy, p), x2 = mulm(qx, qx, p), x3 = mulm(x2, qx, p),
         threeX = addm(addm(qx, qx, p), qx, p), rhs = addm(subm(x3, threeX, p), bb, p);
    if (!eq(lhs, rhs))
        return XT_ERR_VERIFY;
    N256 r, s;
    if (!parse_sig(signature, &r, &s) || zero(r) || zero(s) || cmp(r, n) >= 0 || cmp(s, n) >= 0)
        return XT_ERR_VERIFY;
    xt_u8 hd[32];
    Sha256 h;
    h.update(message.data, message.size);
    h.final(hd);
    N256 z = be(hd, 32);
    if (cmp(z, n) >= 0)
        subraw(z, n);
    N256 nm2 = n;
    subraw(nm2, small(2));
    N256 w = powm(s, nm2, n), u1 = mulm(z, w, n), u2 = mulm(r, w, n);
    JP a = scalar(u1, gx, gy, p), b = scalar(u2, qx, qy, p), sum = addj(a, b, p);
    N256 x;
    if (!affine_x(sum, p, &x))
        return XT_ERR_VERIFY;
    while (cmp(x, n) >= 0)
        subraw(x, n);
    return eq(x, r) ? XT_OK : XT_ERR_VERIFY;
}

} // namespace xboxtls
