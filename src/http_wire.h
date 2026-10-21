#pragma once
#include <stddef.h>

// Reusable, bounded HTTP/1.1 framing for the verified TLS transport.
// This module does not create sockets, disable TLS, or send account details.
namespace http_wire {
enum { MAX_HEADERS = 8192, MAX_BODY = 1024*1024 };
struct Response {
    int status;
    size_t body_offset;
    size_t content_length;
    bool has_length;
    bool chunked;
};
bool make_request(const char* method, const char* host, const char* path,
                  const char* content_type, const char* body, size_t body_length,
                  char* out, size_t capacity, size_t* written);
// Same framing, with the required Xbox Live API contract header.
// Use only for an allowlisted, verified-TLS Xbox endpoint.
bool make_xbox_json_request(const char* host,const char* path,
                            const char* body,size_t body_length,
                            char* out,size_t capacity,size_t* written);
bool make_cloud_login_request(const char* body,size_t bytes,
                              char* out,size_t capacity,size_t* written);
bool make_cloud_session_request(const char* method,const char* host,
                                const char* path,const char* bearer,
                                const char* body,size_t bytes,
                                char* out,size_t capacity,size_t* written);
bool parse_response(const char* data, size_t length, Response* response);
// Framed responses finish without waiting for connection close.
bool response_complete(const char* data,size_t length);
bool decode_chunked(const char* data,size_t length,char* out,
                    size_t capacity,size_t* decoded);
bool form_escape(const char* input, char* out, size_t capacity);
}
