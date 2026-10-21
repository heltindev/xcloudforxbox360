#include "platform.h"
#include "bitmap_font.h"
#include "embedded_font_resource.h"
#include "embedded_font_xpr.h"
#include <xtl.h>
#include <d3d9.h>
#include <AtgApp.h>
#include <AtgFont.h>

// Smooth XPR is linked into the XEX; bitmap is a second, built-in fallback.
namespace {
    ATG::Font g_font;
    bool g_font_ready = false;
    IDirect3D9* g_d3d = 0;
    IDirect3DDevice9* g_device = 0;
    WORD g_previous[4] = {0, 0, 0, 0};
    DWORD g_pad_repeat[4][4] = {{0}};

    // GR33N-inspired controller key repeat, native Xbox 360 GetTickCount.
    bool dpad_event(WORD held, WORD previous, WORD mask,
                    DWORD now, DWORD& next_tick) {
        if (!(held & mask)) { next_tick=0;return false; }
        if (!(previous & mask)) { next_tick=now+330u;return true; }
        if ((LONG)(now-next_tick)>=0) {
            next_tick=now+85u;
            return true;
        }
        return false;
    }
    bool g_scene = false;
    int g_width = 1280;
    int g_height = 720;
}

namespace platform {

bool initialize() {
    debug_log("[XCloud360] Starting Xbox360 D3D renderer / embedded smooth font.\n");
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d) return false;

    D3DPRESENT_PARAMETERS p;
    ZeroMemory(&p, sizeof(p));
    p.BackBufferFormat = D3DFMT_A8R8G8B8;
    p.BackBufferCount = 1;
    p.MultiSampleType = D3DMULTISAMPLE_NONE;
    p.SwapEffect = D3DSWAPEFFECT_DISCARD;
    p.Windowed = FALSE;
    p.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    // Prefer native 1080p for sharper D3D text and shape edges where the
    // console display and title-mode support it. Retain 720p/480p fallbacks
    // for older TVs/RGH dashboards. UI coordinates remain 1280x720 logical.
    static const int modes[][2] = {
        {1920,1080}, {1280,720}, {640,480}
    };
    HRESULT result = E_FAIL;
    bool created = false;
    for (int index=0;index<3;++index) {
        p.BackBufferWidth=(UINT)modes[index][0];
        p.BackBufferHeight=(UINT)modes[index][1];
        result=g_d3d->CreateDevice(
            0,D3DDEVTYPE_HAL,0,D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &p,&g_device);
        if(SUCCEEDED(result) && g_device) {
            created=true;
            if(index==0)debug_log("[XCloud360] Rendering at 1920x1080.\n");
            else if(index==1)debug_log("[XCloud360] Rendering at 1280x720.\n");
            else debug_log("[XCloud360] Rendering at 640x480 (scaled).\n");
            break;
        }
        if(g_device) {g_device->Release();g_device=0;}
    }
    if(!created) {
        debug_log("[XCloud360] D3D9 device creation failed at every resolution.\n");
        shutdown();
        return false;
    }
    g_width = (int)p.BackBufferWidth;
    g_height = (int)p.BackBufferHeight;
    ATG::g_pd3dDevice = reinterpret_cast<ATG::D3DDevice*>(g_device);
    // The smooth font is linked directly into the XEX. No Media/Fonts,
    // title directory, external resource or filesystem mount required.
    g_font_ready = create_embedded_font(g_font);
    if (g_font_ready)
        debug_log("[XCloud360] Segoe smooth font loaded internally from XEX.\n");
    else
        debug_log("[XCloud360] Internal XPR loading failed, builtin bitmap fallback.\n");
    debug_log("[XCloud360] Renderer ready.\n");
    return true;
}

void shutdown() {
    destroy_embedded_font(g_font);
    g_font_ready = false;
    ATG::g_pd3dDevice = 0;
    g_scene = false;
    if (g_device) { g_device->Release(); g_device = 0; }
    if (g_d3d) { g_d3d->Release(); g_d3d = 0; }
}

bool pump() { return true; }

InputState read_input() {
    InputState state = {false, false, false, false, false,
                        false, false, false, false,
                        false, false, false};
    const DWORD tick=GetTickCount();
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
        state.x_pressed |= (buttons & XINPUT_GAMEPAD_X) &&
                           !(g_previous[port] & XINPUT_GAMEPAD_X);
        state.y_pressed |= (buttons & XINPUT_GAMEPAD_Y) &&
                           !(g_previous[port] & XINPUT_GAMEPAD_Y);
        state.up_pressed |= dpad_event(buttons,g_previous[port],
                            XINPUT_GAMEPAD_DPAD_UP,tick,g_pad_repeat[port][0]);
        state.down_pressed |= dpad_event(buttons,g_previous[port],
                              XINPUT_GAMEPAD_DPAD_DOWN,tick,g_pad_repeat[port][1]);
        state.left_pressed |= dpad_event(buttons,g_previous[port],
                              XINPUT_GAMEPAD_DPAD_LEFT,tick,g_pad_repeat[port][2]);
        state.right_pressed |= dpad_event(buttons,g_previous[port],
                               XINPUT_GAMEPAD_DPAD_RIGHT,tick,g_pad_repeat[port][3]);
        state.lb_pressed |= (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) &&
                            !(g_previous[port] & XINPUT_GAMEPAD_LEFT_SHOULDER);
        state.rb_pressed |= (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) &&
                            !(g_previous[port] & XINPUT_GAMEPAD_RIGHT_SHOULDER);
        state.start_pressed |= (buttons & XINPUT_GAMEPAD_START) &&
                               !(g_previous[port] & XINPUT_GAMEPAD_START);
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
        const float sx = (float)g_width / 1280.0f;
        const float sy = (float)g_height / 720.0f;
        // Rasterize glyphs at 42 pt locally but keep the SAME intended
        // on-screen point size as the original 24-pt font. Downsampling a
        // higher-density atlas produces crisper edges at 1080p.
        const float density=(float)g_embedded_font_design_points/24.0f;
        const float fontScaleX = scale / (1.80f*density) * sx;
        const float fontScaleY = scale / (1.80f*density) * sy;
        g_font.SetScaleFactors(fontScaleX, fontScaleY);
        g_font.Begin();
        g_font.DrawText(x * sx, y * sy, color, converted);
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
