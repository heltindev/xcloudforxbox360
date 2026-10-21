#pragma once
#include <stddef.h>

// Strict, bounded parser for flat OAuth JSON response objects.
// No heap allocations. Rejects malformed data instead of trusting it.
namespace json_fields {
bool string(const char* json, size_t size, const char* field,
            char* output, size_t capacity);
bool uint_value(const char* json, size_t size, const char* field,
                unsigned int* output);
}
