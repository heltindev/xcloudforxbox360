#include "xboxtls/sha256.h"
#include <string.h>

namespace xboxtls {

static xt_u32 rotr(xt_u32 x, xt_u32 n) {
    return (x >> n) | (x << (32 - n));
}
static xt_u32 ch(xt_u32 x, xt_u32 y, xt_u32 z) {
    return (x & y) ^ (~x & z);
}
static xt_u32 maj(xt_u32 x, xt_u32 y, xt_u32 z) {
    return (x & y) ^ (x & z) ^ (y & z);
}
static xt_u32 bsig0(xt_u32 x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}
static xt_u32 bsig1(xt_u32 x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}
static xt_u32 ssig0(xt_u32 x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}
static xt_u32 ssig1(xt_u32 x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static const xt_u32 K[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
    0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
    0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
    0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
    0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
    0xc67178f2U};

Sha256::Sha256() {
    reset();
}

void Sha256::reset() {
    state_[0] = 0x6a09e667U;
    state_[1] = 0xbb67ae85U;
    state_[2] = 0x3c6ef372U;
    state_[3] = 0xa54ff53aU;
    state_[4] = 0x510e527fU;
    state_[5] = 0x9b05688cU;
    state_[6] = 0x1f83d9abU;
    state_[7] = 0x5be0cd19U;
    total_bytes_ = 0;
    buffered_ = 0;
    memset(buffer_, 0, sizeof(buffer_));
}

void Sha256::transform(const xt_u8 block[64]) {
    xt_u32 w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = ((xt_u32)block[i * 4] << 24) | ((xt_u32)block[i * 4 + 1] << 16) |
               ((xt_u32)block[i * 4 + 2] << 8) | block[i * 4 + 3];
    for (int i = 16; i < 64; ++i)
        w[i] = ssig1(w[i - 2]) + w[i - 7] + ssig0(w[i - 15]) + w[i - 16];
    xt_u32 a = state_[0], b = state_[1], c = state_[2], d = state_[3], e = state_[4], f = state_[5],
           g = state_[6], h = state_[7];
    for (int i = 0; i < 64; ++i) {
        xt_u32 t1 = h + bsig1(e) + ch(e, f, g) + K[i] + w[i];
        xt_u32 t2 = bsig0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void Sha256::update(const void* data, size_t len) {
    const xt_u8* p = (const xt_u8*)data;
    total_bytes_ += (xt_u64)len;
    while (len) {
        size_t take = 64 - buffered_;
        if (take > len)
            take = len;
        memcpy(buffer_ + buffered_, p, take);
        buffered_ += take;
        p += take;
        len -= take;
        if (buffered_ == 64) {
            transform(buffer_);
            buffered_ = 0;
        }
    }
}

void Sha256::final(xt_u8 out[32]) {
    xt_u64 bits = total_bytes_ * 8;
    buffer_[buffered_++] = 0x80;
    if (buffered_ > 56) {
        while (buffered_ < 64)
            buffer_[buffered_++] = 0;
        transform(buffer_);
        buffered_ = 0;
    }
    while (buffered_ < 56)
        buffer_[buffered_++] = 0;
    for (int i = 7; i >= 0; --i)
        buffer_[buffered_++] = (xt_u8)(bits >> (i * 8));
    transform(buffer_);
    for (int i = 0; i < 8; ++i) {
        out[i * 4] = (xt_u8)(state_[i] >> 24);
        out[i * 4 + 1] = (xt_u8)(state_[i] >> 16);
        out[i * 4 + 2] = (xt_u8)(state_[i] >> 8);
        out[i * 4 + 3] = (xt_u8)state_[i];
    }
}

void sha256(ByteSpan in, xt_u8 out[32]) {
    Sha256 s;
    s.update(in.data, in.size);
    s.final(out);
}

} // namespace xboxtls
