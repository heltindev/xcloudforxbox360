#include "xboxtls/tls13.h"
namespace xboxtls {
TranscriptHash::TranscriptHash() : sha_() {}
void TranscriptHash::reset() {
    sha_.reset();
}
void TranscriptHash::update(ByteSpan b) {
    if (b.size)
        sha_.update(b.data, b.size);
}
void TranscriptHash::snapshot(xt_u8 out[32]) const {
    Sha256 copy = sha_;
    copy.final(out);
}
} // namespace xboxtls
