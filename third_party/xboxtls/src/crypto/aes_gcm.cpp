#include "xboxtls/aes_gcm.h"
#include <string.h>

namespace xboxtls {

static const xt_u8 sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

static xt_u8 xtime(xt_u8 x) {
    return (xt_u8)((x << 1) ^ ((x & 0x80) ? 0x1b : 0));
}
static void add_round_key(xt_u8 s[16], const xt_u8* rk) {
    for (int i = 0; i < 16; ++i)
        s[i] ^= rk[i];
}
static void sub_bytes(xt_u8 s[16]) {
    for (int i = 0; i < 16; ++i)
        s[i] = sbox[s[i]];
}
static void shift_rows(xt_u8 s[16]) {
    xt_u8 t[16];
    memcpy(t, s, 16);
    s[0] = t[0];
    s[4] = t[4];
    s[8] = t[8];
    s[12] = t[12];
    s[1] = t[5];
    s[5] = t[9];
    s[9] = t[13];
    s[13] = t[1];
    s[2] = t[10];
    s[6] = t[14];
    s[10] = t[2];
    s[14] = t[6];
    s[3] = t[15];
    s[7] = t[3];
    s[11] = t[7];
    s[15] = t[11];
}
static void mix_columns(xt_u8 s[16]) {
    for (int c = 0; c < 4; ++c) {
        int i = c * 4;
        xt_u8 a = s[i], b = s[i + 1], d = s[i + 2], e = s[i + 3];
        xt_u8 x = (xt_u8)(a ^ b ^ d ^ e);
        s[i] ^= x ^ xtime((xt_u8)(a ^ b));
        s[i + 1] ^= x ^ xtime((xt_u8)(b ^ d));
        s[i + 2] ^= x ^ xtime((xt_u8)(d ^ e));
        s[i + 3] ^= x ^ xtime((xt_u8)(e ^ a));
    }
}
static void key_expand(const xt_u8 key[16], xt_u8 rk[176]) {
    memcpy(rk, key, 16);
    int bytes = 16;
    xt_u8 rcon = 1;
    xt_u8 t[4];
    while (bytes < 176) {
        for (int i = 0; i < 4; ++i)
            t[i] = rk[bytes - 4 + i];
        if ((bytes & 15) == 0) {
            xt_u8 q = t[0];
            t[0] = sbox[t[1]];
            t[1] = sbox[t[2]];
            t[2] = sbox[t[3]];
            t[3] = sbox[q];
            t[0] ^= rcon;
            rcon = xtime(rcon);
        }
        for (int i = 0; i < 4; ++i) {
            rk[bytes] = (xt_u8)(rk[bytes - 16] ^ t[i]);
            ++bytes;
        }
    }
}
static void aes_block(const xt_u8 key[16], const xt_u8 in[16], xt_u8 out[16]) {
    xt_u8 rk[176], s[16];
    key_expand(key, rk);
    memcpy(s, in, 16);
    add_round_key(s, rk);
    for (int r = 1; r < 10; ++r) {
        sub_bytes(s);
        shift_rows(s);
        mix_columns(s);
        add_round_key(s, rk + 16 * r);
    }
    sub_bytes(s);
    shift_rows(s);
    add_round_key(s, rk + 160);
    memcpy(out, s, 16);
    memset(rk, 0, sizeof(rk));
    memset(s, 0, sizeof(s));
}
static void xor16(xt_u8 a[16], const xt_u8 b[16]) {
    for (int i = 0; i < 16; ++i)
        a[i] ^= b[i];
}
static void shr1(xt_u8 v[16]) {
    xt_u8 carry = 0;
    for (int i = 0; i < 16; ++i) {
        xt_u8 n = (xt_u8)(v[i] & 1);
        v[i] = (xt_u8)((v[i] >> 1) | (carry << 7));
        carry = n;
    }
}
static void gfmul(const xt_u8 x[16], const xt_u8 y[16], xt_u8 out[16]) {
    xt_u8 z[16] = {0}, v[16];
    memcpy(v, y, 16);
    for (int i = 0; i < 128; ++i) {
        if ((x[i >> 3] >> (7 - (i & 7))) & 1)
            xor16(z, v);
        xt_u8 lsb = (xt_u8)(v[15] & 1);
        shr1(v);
        if (lsb)
            v[0] ^= 0xe1;
    }
    memcpy(out, z, 16);
    memset(z, 0, 16);
    memset(v, 0, 16);
}
static void ghash_block(xt_u8 y[16], const xt_u8 h[16], const xt_u8 b[16]) {
    xt_u8 t[16];
    memcpy(t, y, 16);
    xor16(t, b);
    gfmul(t, h, y);
    memset(t, 0, 16);
}
static void ghash_data(xt_u8 y[16], const xt_u8 h[16], ByteSpan d) {
    size_t off = 0;
    while (off < d.size) {
        xt_u8 b[16] = {0};
        size_t n = d.size - off < 16 ? d.size - off : 16;
        memcpy(b, d.data + off, n);
        ghash_block(y, h, b);
        off += n;
    }
}
static void put_be64(xt_u8* p, xt_u64 v) {
    for (int i = 7; i >= 0; --i) {
        p[i] = (xt_u8)v;
        v >>= 8;
    }
}
static void inc32(xt_u8 c[16]) {
    for (int i = 15; i >= 12; --i) {
        if (++c[i])
            break;
    }
}
static void ctr_crypt(const xt_u8 key[16], const xt_u8 j0[16], ByteSpan in, MutableByteSpan out) {
    xt_u8 c[16], ks[16];
    memcpy(c, j0, 16);
    size_t off = 0;
    while (off < in.size) {
        inc32(c);
        aes_block(key, c, ks);
        size_t n = in.size - off < 16 ? in.size - off : 16;
        for (size_t i = 0; i < n; ++i)
            out.data[off + i] = (xt_u8)(in.data[off + i] ^ ks[i]);
        off += n;
    }
    memset(c, 0, 16);
    memset(ks, 0, 16);
}
static void auth_tag(const xt_u8 key[16], const xt_u8 nonce[12], ByteSpan aad, ByteSpan ciphertext,
                     xt_u8 tag[16]) {
    xt_u8 zero[16] = {0}, h[16], j0[16] = {0}, y[16] = {0}, lens[16], e0[16];
    aes_block(key, zero, h);
    memcpy(j0, nonce, 12);
    j0[15] = 1;
    ghash_data(y, h, aad);
    ghash_data(y, h, ciphertext);
    put_be64(lens, (xt_u64)aad.size * 8);
    put_be64(lens + 8, (xt_u64)ciphertext.size * 8);
    ghash_block(y, h, lens);
    aes_block(key, j0, e0);
    for (int i = 0; i < 16; ++i)
        tag[i] = (xt_u8)(y[i] ^ e0[i]);
    memset(h, 0, 16);
    memset(y, 0, 16);
    memset(e0, 0, 16);
}
static bool tag_equal(const xt_u8 a[16], const xt_u8 b[16]) {
    xt_u8 d = 0;
    for (int i = 0; i < 16; ++i)
        d |= (xt_u8)(a[i] ^ b[i]);
    return d == 0;
}

Error aes128_gcm_encrypt(const xt_u8 key[16], const xt_u8 nonce[12], ByteSpan aad,
                         ByteSpan plaintext, MutableByteSpan ciphertext, xt_u8 tag[16]) {
    if (!key || !nonce || !tag || (plaintext.size && !plaintext.data) || (aad.size && !aad.data) ||
        (ciphertext.size && !ciphertext.data))
        return XT_ERR_INVALID_ARGUMENT;
    if (ciphertext.size < plaintext.size)
        return XT_ERR_BUFFER_TOO_SMALL;
    xt_u8 j0[16] = {0};
    memcpy(j0, nonce, 12);
    j0[15] = 1;
    ctr_crypt(key, j0, plaintext, ciphertext);
    auth_tag(key, nonce, aad, ByteSpan(ciphertext.data, plaintext.size), tag);
    return XT_OK;
}
Error aes128_gcm_decrypt(const xt_u8 key[16], const xt_u8 nonce[12], ByteSpan aad,
                         ByteSpan ciphertext, const xt_u8 tag[16], MutableByteSpan plaintext) {
    if (!key || !nonce || !tag || (ciphertext.size && !ciphertext.data) ||
        (aad.size && !aad.data) || (plaintext.size && !plaintext.data))
        return XT_ERR_INVALID_ARGUMENT;
    if (plaintext.size < ciphertext.size)
        return XT_ERR_BUFFER_TOO_SMALL;
    xt_u8 expected[16];
    auth_tag(key, nonce, aad, ciphertext, expected);
    if (!tag_equal(expected, tag)) {
        memset(expected, 0, 16);
        return XT_ERR_VERIFY;
    }
    xt_u8 j0[16] = {0};
    memcpy(j0, nonce, 12);
    j0[15] = 1;
    ctr_crypt(key, j0, ciphertext, plaintext);
    memset(expected, 0, 16);
    return XT_OK;
}

} // namespace xboxtls
