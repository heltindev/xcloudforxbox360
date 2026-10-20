#pragma once
#include <stddef.h>

// Xbox Cloud Gaming data-channel message formats observed in GR33N.
// New implementation targeting the Xbox 360 PowerPC.
// Only formatters/parsers: NO session creation, authentication or WebRTC.
namespace xcloud_control {
bool handshake(char* out, size_t capacity, const char* message_id);
bool handshake_ack(const char* json, size_t size);
bool gamepad_changed(char* out,size_t capacity,
                     unsigned int controller_index,bool added);
bool resolution(char* out,size_t capacity,unsigned int height);
bool keyframe_request(char* out,size_t capacity);
// Produce a single escaped JSON "Message" envelope containing client
// video constraints. ID must be provided by a trusted session controller.
bool device_capabilities(char* out,size_t capacity,const char* message_id,
                         unsigned int width,unsigned int height,
                         unsigned int fps,unsigned int bitrate_kbps);
}
