#include "platform.h"
#include "bitmap_font.h"
#include <xtl.h>
#include <d3d9.h>
#include <AtgApp.h>
#include <AtgFont.h>

// Smooth Segoe UI XPR for presentation, embedded bitmap as safe fallback.
namespace {
    ATG::Font g_font;
    bool g_font_ready = false;
    IDirect3D9* g_d3d = 0;
    IDirect3DDevice9* g_device = 0;
    WORD g_previous[4] = {0, 0, 0, 0};
    bool g_scene = false;
    int g_width = 1280;
    int g_height = 720;
}

namespace platform {

bool initialize() {
    debug_log("[XCloud360] Starting standalone D3D9 renderer / bitmap text.\n");
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d) return false;

    D3DPRESENT_PARAMETERS p;
    ZeroMemory(&p, sizeof(p));
    p.BackBufferWidth = 1280;
    p.BackBufferHeight = 720;
    p.BackBufferFormat = D3DFMT_A8R8G8B8;
    p.BackBufferCount = 1;
    p.MultiSampleType = D3DMULTISAMPLE_NONE;
    p.SwapEffect = D3DSWAPEFFECT_DISCARD;
    p.Windowed = FALSE;
    p.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    // Prefer 720p on Xbox 360 and fall back to 480p for older displays.
    HRESULT result = g_d3d->CreateDevice(
        0, D3DDEVTYPE_HAL, 0, D3DCREATE_HARDWARE_VERTEXPROCESSING, &p, &g_device);
    if (FAILED(result)) {
        p.BackBufferWidth = 640;
        p.BackBufferHeight = 480;
        result = g_d3d->CreateDevice(
            0, D3DDEVTYPE_HAL, 0, D3DCREATE_HARDWARE_VERTEXPROCESSING, &p, &g_device);
        if (FAILED(result)) {
            debug_log("[XCloud360] D3D9 CreateDevice failed (720p and 480p).\n");
            shutdown();
            return false;
        }
    }
    g_width = (int)p.BackBufferWidth;
    g_height = (int)p.BackBufferHeight;
    ATG::g_pd3dDevice = reinterpret_cast<ATG::D3DDevice*>(g_device);
    // In Aurora/XeXMenu, game: commonly points to the title directory.
    // D: is a fallback on consoles launched by alternate dashboards.
    const char* paths[] = {
        "game:\\Media\\Fonts\\SegoeUI_24.xpr",
        "D:\\Media\\Fonts\\SegoeUI_24.xpr"
    };
    for (int i = 0; i < 2 && !g_font_ready; ++i)
        g_font_ready = SUCCEEDED(g_font.Create(paths[i]));
    if (g_font_ready)
        debug_log("[XCloud360] Segoe smooth font loaded.\n");
    else
        debug_log("[XCloud360] Smooth font missing, embedded bitmap fallback.\n");
    debug_log("[XCloud360] Renderer ready.\n");
    return true;
}

void shutdown() {
    if (g_font_ready) { g_font.Destroy(); g_font_ready = false; }
    ATG::g_pd3dDevice = 0;
    g_scene = false;
    if (g_device) { g_device->Release(); g_device = 0; }
    if (g_d3d) { g_d3d->Release(); g_d3d = 0; }
}

bool pump() { return true; }

InputState read_input() {
    InputState state = {false, false, false};
    for (DWORD port = 0; port < 4; ++port) {
        XINPUT_STATE raw;
        ZeroMemory(&raw, sizeof(raw));
        const WORD buttons =
            XInputGetState(port, &raw) == ERROR_SUCCESS ? raw.Gamepad.wButtons : 0;
        state.a_pressed |= (buttons & XINPUT_GAMEPAD_A) &&
                           !(g_previous[port] & XINPUT_GAMEPAD_A);
        state.b_pressed |= (buttons & XINPUT_GAMEPAD_B) &&
                           !(g_previous[port] & XINPUT_GAMEPAD_B);
        state.back_pressed |= (buttons & XINPUT_GAMEPAD_BACK) &&
                              !(g_previous[port] & XINPUT_GAMEPAD_BACK);
        g_previous[port] = buttons;
    }
    return state;
}

void clear(unsigned char red, unsigned char green, unsigned char blue) {
    if (!g_device) return;
    if (g_scene) { g_device->EndScene(); g_scene = false; }
    g_device->Clear(0, 0, D3DCLEAR_TARGET,
                    D3DCOLOR_XRGB(red,green,blue), 1.0f, 0);
    g_scene = SUCCEEDED(g_device->BeginScene());
}

void draw_rect(float x, float y, float width, float height, unsigned int color) {
    if (!g_device || !g_scene || width <= 0 || height <= 0) return;
    const float sx = (float)g_width / 1280.0f;
    const float sy = (float)g_height / 720.0f;
    LONG x1 = (LONG)(x * sx);
    LONG y1 = (LONG)(y * sy);
    LONG x2 = (LONG)((x + width) * sx + 0.5f);
    LONG y2 = (LONG)((y + height) * sy + 0.5f);
    if (x2 <= 0 || y2 <= 0 || x1 >= g_width || y1 >= g_height) return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > g_width) x2 = g_width;
    if (y2 > g_height) y2 = g_height;
    if (x2 <= x1 || y2 <= y1) return;
    D3DRECT rectangle = {x1,y1,x2,y2};
    g_device->Clear(1, &rectangle, D3DCLEAR_TARGET, color, 1.0f, 0);
}

void draw_text(float x,float y,float scale,unsigned int color,const char* text) {
    if (!text) return;
    if (g_font_ready) {
        WCHAR converted[221];
        int len = 0;
        for (; len < 220 && text[len] != 0; ++len)
            converted[len] = (WCHAR)(unsigned char)text[len];
        converted[len] = 0;
        const float fontScale = scale / 1.80f;
        g_font.SetScaleFactors(fontScale, fontScale);
        g_font.Begin();
        g_font.DrawText(x, y, color, converted);
        g_font.End();
    } else {
        bitmap_draw_text(x,y,scale,color,text);
    }
}

void present() {
    if (!g_device) return;
    if (g_scene) { g_device->EndScene(); g_scene = false; }
    g_device->Present(0, 0, 0, 0);
}

void debug_log(const char* message) {
    if (message) OutputDebugStringA(message);
}

} // namespace platform
