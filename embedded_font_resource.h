#pragma once
#include <xtl.h>
#include <AtgFont.h>

// Loads the XPR resource straight from bytes linked into XCloud360.xex,
// without reading game:\\Media\\Fonts or touching the filesystem.
bool create_embedded_font(ATG::Font& font);
void destroy_embedded_font(ATG::Font& font);
