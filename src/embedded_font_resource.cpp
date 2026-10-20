#include "embedded_font_resource.h"
#include "embedded_font_xpr.h"
#include <AtgResource.h>
#include <xgraphics.h>
#include <string.h>


// This is a small, application-local in-memory XPR loader. Unlike
// ATG::PackedResource::Create(filename), it never opens an external file.
// Only XPR2 resources produced by the local Xbox 360 SDK Bundler are accepted.
namespace {
    struct XprHeader {
        DWORD magic;
        DWORD headerSize;
        DWORD dataSize;
    };
    const DWORD kXpr2Magic = 0x58505232;
    BYTE* g_headers = NULL;
    BYTE* g_textureData = NULL;

    // Equivalent to the helper in the SDK AtgResource.cpp; the SDK helper
    // is private/static there and cannot be linked from our translation unit.
    BYTE* allocate_texture_data(DWORD size) {
        return static_cast<BYTE*>(XMemAlloc(
            size, MAKE_XALLOC_ATTRIBUTES(
                0, 0, 0, 0, eXALLOCAllocatorId_GameMax,
                XALLOC_PHYSICAL_ALIGNMENT_4K,
                XALLOC_MEMPROTECT_WRITECOMBINE, 0,
                XALLOC_MEMTYPE_PHYSICAL)));
    }

    void release_texture_data(BYTE* data) {
        XMemFree(data, MAKE_XALLOC_ATTRIBUTES(
            0, 0, 0, 0, eXALLOCAllocatorId_GameMax,
            0, 0, 0, XALLOC_MEMTYPE_PHYSICAL));
    }

    void free_storage() {
        if (g_textureData) {
            release_texture_data(g_textureData);
            g_textureData = NULL;
        }
        delete[] g_headers;
        g_headers = NULL;
    }
    bool name_equals(const CHAR* str, const BYTE* begin, DWORD length,
                     const char* expected) {
        const BYTE* p = reinterpret_cast<const BYTE*>(str);
        if (p < begin || p >= begin + length) return false;
        const DWORD max = (DWORD)((begin + length) - p);
        const DWORD needed = (DWORD)strlen(expected) + 1;
        return needed <= max && memcmp(str,expected,needed) == 0;
    }
}

bool create_embedded_font(ATG::Font& font) {
    if (g_headers || g_textureData) return false;
    if (g_embedded_font_xpr_size < sizeof(XprHeader)) return false;

    const XprHeader* header =
        reinterpret_cast<const XprHeader*>(g_embedded_font_xpr);
    if (header->magic != kXpr2Magic ||
        header->headerSize < 4 + 2*sizeof(ATG::RESOURCE) ||
        header->headerSize > (2u << 20) ||
        header->dataSize == 0 || header->dataSize > (2u << 20) ||
        header->headerSize > g_embedded_font_xpr_size - sizeof(XprHeader) ||
        header->dataSize >
            g_embedded_font_xpr_size - sizeof(XprHeader) - header->headerSize) {
        return false;
    }

    g_headers = new BYTE[header->headerSize];
    if (!g_headers) return false;
    g_textureData = allocate_texture_data(header->dataSize);
    if (!g_textureData) { free_storage(); return false; }

    memcpy(g_headers,g_embedded_font_xpr + sizeof(XprHeader),header->headerSize);
    memcpy(g_textureData,g_embedded_font_xpr + sizeof(XprHeader) +
           header->headerSize,header->dataSize);

    const DWORD count = *reinterpret_cast<DWORD*>(g_headers);
    if (count == 0 || count > 128 ||
        count > (header->headerSize - 4) / sizeof(ATG::RESOURCE)) {
        free_storage(); return false;
    }

    ATG::RESOURCE* entries = reinterpret_cast<ATG::RESOURCE*>(g_headers+4);
    D3DTexture* fontTexture = NULL;
    const void* fontData = NULL;
    for (DWORD i = 0; i < count; ++i) {
        ATG::RESOURCE& item = entries[i];
        const DWORD nameOffset = (DWORD)item.strName;  // Offset in XPR2.
        if (nameOffset >= header->headerSize ||
            item.dwOffset >= header->headerSize ||
            item.dwSize > header->headerSize - item.dwOffset) {
            free_storage(); return false;
        }
        CHAR* name = reinterpret_cast<CHAR*>(g_headers + nameOffset);
        if ((item.dwType & 0xffff0000) ==
            (ATG::RESOURCETYPE_TEXTURE & 0xffff0000)) {
            if (item.dwSize < sizeof(D3DTexture)) { free_storage(); return false; }
            D3DTexture* texture =
                reinterpret_cast<D3DTexture*>(g_headers+item.dwOffset);
            XGOffsetBaseTextureAddress(texture,g_textureData,g_textureData);
            if (name_equals(name,g_headers,header->headerSize,"FontTexture"))
                fontTexture = texture;
        } else if (name_equals(name,g_headers,header->headerSize,"FontData")) {
            fontData = g_headers + item.dwOffset;
        }
    }

    if (!fontTexture || !fontData || FAILED(font.Create(fontTexture,fontData))) {
        font.Destroy();
        free_storage();
        return false;
    }
    return true;
}

void destroy_embedded_font(ATG::Font& font) {
    font.Destroy();
    free_storage();
}
