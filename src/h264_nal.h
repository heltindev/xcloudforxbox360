#pragma once
#include <stddef.h>

// H.264 Annex B NAL unit scanning only. NOT an H.264 decoder.
namespace h264 {
struct NalUnit {
    size_t offset;
    size_t length;
    unsigned char nal_type;
    unsigned char ref_idc;
};
// cursor starts at 0; advances to the next unit on each successful call.
bool next_nal(const unsigned char* data,size_t size,size_t* cursor,NalUnit* out);
}
