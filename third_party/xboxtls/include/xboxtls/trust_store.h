#ifndef XBOXTLS_TRUST_STORE_H
#define XBOXTLS_TRUST_STORE_H

#include "error.h"
#include "types.h"

namespace xboxtls {

class NativeServerAuthVerifier;

// Compact, allocation-free trust-store format:
//   4 bytes magic: "XTS1"
//   2 bytes big-endian certificate count
//   repeated count times:
//     3 bytes big-endian DER length
//     DER certificate bytes
// The caller must keep the blob alive for as long as the verifier uses it.
Error load_compact_trust_store(ByteSpan blob, NativeServerAuthVerifier* verifier,
                               size_t* loaded_count);

} // namespace xboxtls

#endif
