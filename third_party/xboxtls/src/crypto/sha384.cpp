#include "xboxtls/sha384.h"
#include <string.h>

namespace xboxtls {
static xt_u64 rotr64(xt_u64 x, unsigned n) {
    return (x >> n) | (x << (64 - n));
}
static const xt_u64 K512[80] = {
    0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
    0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
    0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
    0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL
};

Sha384::Sha384() { reset(); }
void Sha384::reset() {
    static const xt_u64 IV[8] = {
        0xcbbb9d5dc1059ed8ULL,0x629a292a367cd507ULL,
        0x9159015a3070dd17ULL,0x152fecd8f70e5939ULL,
        0x67332667ffc00b31ULL,0x8eb44a8768581511ULL,
        0xdb0c2e0d64f98fa7ULL,0x47b5481dbefa4fa4ULL
    };
    memcpy(state_,IV,sizeof(state_));
    total_bytes_=0;
    buffered_=0;
    memset(buffer_,0,sizeof(buffer_));
}
void Sha384::transform(const xt_u8 block[128]) {
    xt_u64 w[80];
    for(int i=0;i<16;++i) {
        xt_u64 value=0;
        for(int j=0;j<8;++j)value=(value<<8)|block[8*i+j];
        w[i]=value;
    }
    for(int i=16;i<80;++i){
        xt_u64 x=w[i-15],y=w[i-2];
        xt_u64 s0=rotr64(x,1)^rotr64(x,8)^(x>>7);
        xt_u64 s1=rotr64(y,19)^rotr64(y,61)^(y>>6);
        w[i]=w[i-16]+s0+w[i-7]+s1;
    }
    xt_u64 a=state_[0],b=state_[1],c=state_[2],d=state_[3];
    xt_u64 e=state_[4],f=state_[5],g=state_[6],h=state_[7];
    for(int i=0;i<80;++i){
        xt_u64 s1=rotr64(e,14)^rotr64(e,18)^rotr64(e,41);
        xt_u64 ch=(e&f)^(~e&g);
        xt_u64 t1=h+s1+ch+K512[i]+w[i];
        xt_u64 s0=rotr64(a,28)^rotr64(a,34)^rotr64(a,39);
        xt_u64 maj=(a&b)^(a&c)^(b&c);
        xt_u64 t2=s0+maj;
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    state_[0]+=a;state_[1]+=b;state_[2]+=c;state_[3]+=d;
    state_[4]+=e;state_[5]+=f;state_[6]+=g;state_[7]+=h;
    memset(w,0,sizeof(w));
}
void Sha384::update(const void* input, size_t len) {
    if(!len)return;
    if(!input)return;
    const xt_u8* p=(const xt_u8*)input;
    total_bytes_+=(xt_u64)len;
    while(len) {
        size_t n=128-buffered_;
        if(n>len)n=len;
        memcpy(buffer_+buffered_,p,n);
        buffered_+=n;p+=n;len-=n;
        if(buffered_==128) {
            transform(buffer_);
            buffered_=0;
        }
    }
}
void Sha384::final(xt_u8 out[48]) {
    // Full 128-bit big-endian bit count (upper 64 bits derived from bytes).
    xt_u64 hi=total_bytes_>>61;
    xt_u64 lo=total_bytes_<<3;
    buffer_[buffered_++]=0x80;
    if(buffered_>112) {
        while(buffered_<128)buffer_[buffered_++]=0;
        transform(buffer_);
        buffered_=0;
    }
    while(buffered_<112)buffer_[buffered_++]=0;
    for(int i=7;i>=0;--i)buffer_[buffered_++]=(xt_u8)(hi>>(8*i));
    for(int i=7;i>=0;--i)buffer_[buffered_++]=(xt_u8)(lo>>(8*i));
    transform(buffer_);
    for(int i=0;i<6;++i)
        for(int j=0;j<8;++j)
            out[i*8+j]=(xt_u8)(state_[i]>>(56-8*j));
    reset();
}
void sha384(ByteSpan in, xt_u8 out[48]) {
    Sha384 h;
    h.update(in.data,in.size);
    h.final(out);
}
} // namespace xboxtls
