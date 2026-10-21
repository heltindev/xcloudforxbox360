#pragma once
#include <stddef.h>

// Parses the documented GR33N-style regional list, using portable cJSON.
// A response must come from HTTPS with certificate + hostname verification.
// Parsing these records does NOT establish streaming service authorization.
namespace region_catalog {
enum { MAX_REGIONS=20 };
struct Region {
    char name[64];
    char base_uri[192];
    char host[128];
    bool is_default;
};
struct Catalog {
    Region regions[MAX_REGIONS];
    unsigned int count;
    unsigned int default_index;
};
bool parse(const char* json,size_t bytes,Catalog* result);
// Rejects non-HTTPS, embedded userinfo, unknown host suffixes, invalid IPs
// and suspicious URL syntax. Called on authenticated service JSON only.
bool extract_xbox_hostname(const char* url,char* host,size_t capacity);
bool offline_selftest();
}
