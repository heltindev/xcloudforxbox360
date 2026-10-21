#pragma once
#include <stddef.h>

// Offline-only OAuth 2.0 Device Authorization message handling.
// Requires a permitted client_id/scope AND an independently verified TLS
// transport before anyone may enable an actual Microsoft sign-in.
namespace oauth_device {
struct DeviceAuthorization {
    char verification_uri[256];
    char user_code[64];
    char device_code[2048]; // secret, memory-only, never log or commit
    unsigned int expires_in;
    unsigned int interval;
};
bool make_device_form(const char* public_client_id,const char* scope,
                      char* output,size_t capacity);
bool read_device_response(const char* json,size_t length,
                          DeviceAuthorization* result);
void clear(DeviceAuthorization* auth);
}
