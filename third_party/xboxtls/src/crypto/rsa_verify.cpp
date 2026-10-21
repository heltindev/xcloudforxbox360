#include "xboxtls/native_verifier.h"
#include "xboxtls/sha256.h"
#include "xboxtls/sha384.h"
#include <string.h>

namespace xboxtls {

static const size_t MAX_WORDS = 128; // RSA up to 4096 bits.
struct Nat {
    xt_u32 w[MAX_WORDS + 1];
    size_t n;
    Nat() : n(0) {
        memset(w, 0, sizeof(w));
    }
};

struct Tlv {
    xt_u8 tag;
    ByteSpan value;
    size_t next;
};
static Error tlv(ByteSpan in, size_t off, Tlv* o) {
    if (!o || !in.data || off >= in.size)
        return XT_ERR_BAD_HANDSHAKE;
    size_t p = off;
    o->tag = in.data[p++];
    if (p >= in.size)
        return XT_ERR_BAD_HANDSHAKE;
    xt_u8 lb = in.data[p++];
    size_t len = 0;
    if (!(lb & 0x80))
        len = lb;
    else {
        size_t n = lb & 0x7f;
        if (!n || n > 4 || p + n > in.size || in.data[p] == 0)
            return XT_ERR_BAD_HANDSHAKE;
        for (size_t i = 0; i < n; ++i)
            len = (len << 8) | in.data[p++];
        if (len < 128)
            return XT_ERR_BAD_HANDSHAKE;
    }
    if (len > in.size - p)
        return XT_ERR_BAD_HANDSHAKE;
    o->value = ByteSpan(in.data + p, len);
    o->next = p + len;
    return XT_OK;
}

static void trim(Nat& a) {
    while (a.n && a.w[a.n - 1] == 0)
        --a.n;
}
static int cmp(const Nat& a, const Nat& b) {
    if (a.n != b.n)
        return a.n < b.n ? -1 : 1;
    for (size_t i = a.n; i > 0; --i) {
        if (a.w[i - 1] != b.w[i - 1])
            return a.w[i - 1] < b.w[i - 1] ? -1 : 1;
    }
    return 0;
}
static Nat from_be(ByteSpan s) {
    Nat a;
    size_t words = (s.size + 3) / 4;
    if (words > MAX_WORDS)
        return a;
    a.n = words;
    for (size_t i = 0; i < s.size; ++i) {
        size_t rev = s.size - 1 - i;
        a.w[i / 4] |= ((xt_u32)s.data[rev]) << ((i % 4) * 8);
    }
    trim(a);
    return a;
}
static void sub_inplace(Nat& a, const Nat& b) {
    xt_u64 borrow = 0;
    for (size_t i = 0; i < a.n; ++i) {
        xt_u64 av = a.w[i];
        xt_u64 bv = (i < b.n ? b.w[i] : 0) + borrow;
        if (av >= bv) {
            a.w[i] = (xt_u32)(av - bv);
            borrow = 0;
        } else {
            a.w[i] = (xt_u32)(((xt_u64)1 << 32) + av - bv);
            borrow = 1;
        }
    }
    trim(a);
}
static Nat add_mod(const Nat& a, const Nat& b, const Nat& m) {
    Nat t;
    size_t n = m.n;
    t.n = n + 1;
    xt_u64 carry = 0;
    for (size_t i = 0; i < n; ++i) {
        xt_u64 s = carry + (i < a.n ? a.w[i] : 0) + (i < b.n ? b.w[i] : 0);
        t.w[i] = (xt_u32)s;
        carry = s >> 32;
    }
    t.w[n] = (xt_u32)carry;
    trim(t);
    Nat mm = m;
    if (t.n > m.n || cmp(t, mm) >= 0)
        sub_inplace(t, mm);
    return t;
}
static bool bit(const Nat& a, size_t i) {
    size_t w = i / 32;
    return w < a.n && ((a.w[w] >> (i % 32)) & 1) != 0;
}
static size_t bits(const Nat& a) {
    if (!a.n)
        return 0;
    xt_u32 v = a.w[a.n - 1];
    size_t b = 32;
    while (b && ((v >> (b - 1)) & 1) == 0)
        --b;
    return (a.n - 1) * 32 + b;
}
static Nat mul_mod(Nat a, const Nat& b, const Nat& m) {
    Nat r;
    r.n = 0;
    size_t nb = bits(b);
    for (size_t i = 0; i < nb; ++i) {
        if (bit(b, i))
            r = add_mod(r, a, m);
        a = add_mod(a, a, m);
    }
    return r;
}
static Nat pow_u32_mod(const Nat& a, xt_u32 e, const Nat& m) {
    Nat r;
    r.n = 1;
    r.w[0] = 1;
    Nat x = a;
    while (e) {
        if (e & 1)
            r = mul_mod(r, x, m);
        e >>= 1;
        if (e)
            x = mul_mod(x, x, m);
    }
    return r;
}
static bool to_be_fixed(const Nat& a, xt_u8* out, size_t n) {
    memset(out, 0, n);
    for (size_t i = 0; i < a.n; ++i)
        for (size_t j = 0; j < 4; ++j) {
            size_t k = i * 4 + j;
            if (k < n)
                out[n - 1 - k] = (xt_u8)(a.w[i] >> (8 * j));
        }
    return true;
}

static Error parse_rsa(ByteSpan der, ByteSpan* mod, xt_u32* exp) {
    if (!mod || !exp)
        return XT_ERR_INVALID_ARGUMENT;
    Tlv seq;
    Error e = tlv(der, 0, &seq);
    if (e != XT_OK || seq.tag != 0x30 || seq.next != der.size)
        return XT_ERR_BAD_HANDSHAKE;
    Tlv n;
    e = tlv(seq.value, 0, &n);
    if (e != XT_OK || n.tag != 0x02)
        return XT_ERR_BAD_HANDSHAKE;
    Tlv x;
    e = tlv(seq.value, n.next, &x);
    if (e != XT_OK || x.tag != 0x02 || x.next != seq.value.size)
        return XT_ERR_BAD_HANDSHAKE;
    ByteSpan nv = n.value;
    if (nv.size > 1 && nv.data[0] == 0)
        nv = ByteSpan(nv.data + 1, nv.size - 1);
    if (!nv.size || nv.size > 512)
        return XT_ERR_UNSUPPORTED;
    xt_u32 ev = 0;
    if (!x.value.size || x.value.size > 4)
        return XT_ERR_UNSUPPORTED;
    for (size_t i = 0; i < x.value.size; ++i)
        ev = (ev << 8) | x.value.data[i];
    if (ev < 3 || (ev & 1) == 0)
        return XT_ERR_VERIFY;
    *mod = nv;
    *exp = ev;
    return XT_OK;
}

static Error rsa_public(ByteSpan key, ByteSpan sig, xt_u8* out, size_t* out_len, size_t* out_bits) {
    ByteSpan mod;
    xt_u32 e = 0;
    Error er = parse_rsa(key, &mod, &e);
    if (er != XT_OK)
        return er;
    if (sig.size != mod.size)
        return XT_ERR_VERIFY;
    Nat m = from_be(mod), s = from_be(sig);
    if (!m.n || cmp(s, m) >= 0)
        return XT_ERR_VERIFY;
    Nat r = pow_u32_mod(s, e, m);
    to_be_fixed(r, out, mod.size);
    *out_len = mod.size;
    *out_bits = bits(m);
    return XT_OK;
}

static void hash(ByteSpan in, xt_u8 out[32]) {
    Sha256 s;
    s.update(in.data, in.size);
    s.final(out);
}
static void mgf1(const xt_u8 seed[32], xt_u8* out, size_t n) {
    xt_u32 c = 0;
    size_t p = 0;
    while (p < n) {
        xt_u8 ctr[4] = {(xt_u8)(c >> 24), (xt_u8)(c >> 16), (xt_u8)(c >> 8), (xt_u8)c};
        Sha256 h;
        h.update(seed, 32);
        h.update(ctr, 4);
        xt_u8 d[32];
        h.final(d);
        size_t take = (n - p < 32 ? n - p : 32);
        memcpy(out + p, d, take);
        p += take;
        ++c;
    }
}

Error native_rsa_pss_sha256_verify(ByteSpan key, ByteSpan message, ByteSpan signature) {
    xt_u8 em[512];
    size_t emlen = 0, modbits = 0;
    Error e = rsa_public(key, signature, em, &emlen, &modbits);
    if (e != XT_OK)
        return e;
    const size_t hlen = 32, slen = 32;
    size_t embits = modbits - 1;
    size_t want = (embits + 7) / 8;
    if (emlen != want || emlen < hlen + slen + 2 || em[emlen - 1] != 0xbc)
        return XT_ERR_VERIFY;
    size_t dblen = emlen - hlen - 1;
    const xt_u8* H = em + dblen;
    xt_u8 mask[512];
    mgf1(H, mask, dblen);
    xt_u8 db[512];
    for (size_t i = 0; i < dblen; ++i)
        db[i] = em[i] ^ mask[i];
    unsigned unused = (unsigned)(8 * emlen - embits);
    if (unused) {
        xt_u8 keep = (xt_u8)(0xff >> unused);
        if ((em[0] & ~keep) != 0)
            return XT_ERR_VERIFY;
        db[0] &= keep;
    }
    size_t ps = dblen - slen - 1;
    for (size_t i = 0; i < ps; ++i)
        if (db[i] != 0)
            return XT_ERR_VERIFY;
    if (db[ps] != 1)
        return XT_ERR_VERIFY;
    xt_u8 mh[32];
    hash(message, mh);
    xt_u8 prefix[8];
    memset(prefix, 0, 8);
    Sha256 hh;
    hh.update(prefix, 8);
    hh.update(mh, 32);
    hh.update(db + ps + 1, slen);
    xt_u8 hp[32];
    hh.final(hp);
    xt_u8 diff = 0;
    for (size_t i = 0; i < 32; ++i)
        diff |= hp[i] ^ H[i];
    return diff ? XT_ERR_VERIFY : XT_OK;
}

Error native_rsa_pkcs1_sha256_verify(ByteSpan key, ByteSpan message, ByteSpan signature) {
    xt_u8 em[512];
    size_t n = 0, b = 0;
    Error e = rsa_public(key, signature, em, &n, &b);
    if (e != XT_OK)
        return e;
    (void)b;
    static const xt_u8 di_prefix[] = {0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
                                      0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20};
    xt_u8 d[32];
    hash(message, d);
    size_t tail = sizeof(di_prefix) + 32;
    if (n < 3 + 8 + tail || em[0] != 0 || em[1] != 1)
        return XT_ERR_VERIFY;
    size_t p = 2;
    while (p < n && em[p] == 0xff)
        ++p;
    if (p < 10 || p >= n || em[p++] != 0)
        return XT_ERR_VERIFY;
    if (n - p != tail || memcmp(em + p, di_prefix, sizeof(di_prefix)) != 0 ||
        memcmp(em + p + sizeof(di_prefix), d, 32) != 0)
        return XT_ERR_VERIFY;
    return XT_OK;
}

Error native_rsa_pkcs1_sha384_verify(ByteSpan key, ByteSpan message, ByteSpan signature) {
    // RSASSA-PKCS1-v1_5 with SHA-384 (RFC 8017 / FIPS 180-4).
    // DigestInfo is DER SEQUENCE(AlgorithmIdentifier(id-sha384,NULL),
    //                            OCTET STRING(48-byte digest)).
    static const xt_u8 prefix[] = {
        0x30,0x41,0x30,0x0d,0x06,0x09,0x60,0x86,0x48,0x01,
        0x65,0x03,0x04,0x02,0x02,0x05,0x00,0x04,0x30
    };
    xt_u8 em[512];
    size_t n=0, bits=0;
    Error e=rsa_public(key,signature,em,&n,&bits);
    if(e!=XT_OK)return e;
    (void)bits;
    const size_t tail=sizeof(prefix)+48;
    if(n<3+8+tail || em[0]!=0 || em[1]!=1)
        return XT_ERR_VERIFY;
    size_t p=2;
    while(p<n && em[p]==0xff)++p;
    if(p<10 || p>=n || em[p++]!=0 || n-p!=tail)
        return XT_ERR_VERIFY;
    xt_u8 hash_bytes[48];
    sha384(message,hash_bytes);
    xt_u8 diff=0;
    for(size_t i=0;i<sizeof(prefix);++i)
        diff|=(xt_u8)(em[p+i]^prefix[i]);
    p+=sizeof(prefix);
    for(size_t i=0;i<48;++i)
        diff|=(xt_u8)(em[p+i]^hash_bytes[i]);
    memset(hash_bytes,0,sizeof(hash_bytes));
    memset(em,0,sizeof(em));
    return diff?XT_ERR_VERIFY:XT_OK;
}

} // namespace xboxtls
